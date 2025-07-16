#include <algorithm>
#include <map>
#include <memory>
#include <vector>

#include <plf_list.h>
#include <unordered_dense.h>

#include "net.h"

#include <cage-core/concurrent.h>
#include <cage-core/config.h>
#include <cage-core/debug.h>
#include <cage-core/flatSet.h>
#include <cage-core/math.h> // random
#include <cage-core/networkGinnel.h>
#include <cage-core/serialization.h>

namespace cage
{
	// nomenclature:
	// message - GinnelConnection methods read and write are working with messages
	// packet - system functions work with packets
	// command - messages are fragmented into commands which are grouped into packets

	namespace
	{
		using namespace privat;

		const ConfigUint32 logLevel("cage/ginnel/logLevel", 0);
		const ConfigFloat confSimulatedPacketLoss("cage/ginnel/simulatedPacketLoss", 0);
		const ConfigUint32 confBufferSize("cage/ginnel/systemBufferSize", 1024 * 1024);

#define UDP_LOG(LEVEL, MSG) \
	{ \
		if (logLevel >= (LEVEL)) \
		{ \
			CAGE_LOG(SeverityEnum::Info, "ginnel", Stringizer() + MSG); \
		} \
	}

		struct MemView
		{
			MemView() = default;
			MemView(MemView &&) = default;
			MemView &operator=(MemView &&) = default;

			MemView(const MemView &other) : buffer(other.buffer.share()), offset(other.offset), size(other.size) {}

			MemView &operator=(const MemView &other)
			{
				if (this != &other)
				{
					buffer = other.buffer.share();
					offset = other.offset;
					size = other.size;
				}
				return *this;
			}

			MemView(Holder<MemoryBuffer> buffer, uintPtr offset, uintPtr size) : buffer(std::move(buffer)), offset(offset), size(size) {}

			Deserializer des() const
			{
				Deserializer d(*buffer);
				d.read(offset);
				return d.subview(size);
			}

			Holder<MemoryBuffer> buffer;
			uintPtr offset = 0;
			uintPtr size = 0;
		};

		struct SockGroup : private Immovable
		{
			struct Receiver : private Immovable
			{
				Addr address; // preferred address for responding
				std::vector<MemView> packets;
				uint64 connId = 0;
				uint32 sockIndex = m; // preferred socket for responding
			};

			ankerl::unordered_dense::map<uint64, std::weak_ptr<Receiver>> receivers;
			std::weak_ptr<std::vector<std::shared_ptr<Receiver>>> accepting;
			std::vector<Sock> socks;
			Holder<Mutex> mut = newMutex();

			void applyBufferSizes()
			{
				for (Sock &s : socks)
					s.setBufferSize(confBufferSize);
			}

			void readAll()
			{
				for (uint32 sockIndex = 0; sockIndex < numeric_cast<uint32>(socks.size()); sockIndex++)
				{
					Sock &s = socks[sockIndex];
					if (!s.isValid())
						continue;
					try
					{
						Addr adr;
						while (true)
						{
							auto avail = s.available();
							if (avail < 12)
								break;
							Holder<MemoryBuffer> buff = systemMemory().createHolder<MemoryBuffer>();
							buff->resize(avail);
							uintPtr off = 0;
							while (avail)
							{
								uintPtr siz = s.recvFrom(buff->data() + off, avail, adr);
								CAGE_ASSERT(siz <= avail);
								MemView mv(buff.share(), off, siz);
								avail -= siz;
								off += siz;
								if (siz < 12)
								{
									UDP_LOG(7, "received invalid packet (too small)");
									continue;
								}
								Deserializer des = mv.des();
								{ // read signature
									uint32 sign = 0;
									des >> sign;
									if (sign != CageMagic)
									{
										UDP_LOG(7, "received invalid packet (wrong signature)");
										continue;
									}
								}
								uint64 connId;
								des >> connId;
								auto r = receivers[connId].lock();
								if (r)
								{
									r->packets.push_back(mv);
									if (r->sockIndex == m)
									{
										// set preferred socket and address for responding
										r->sockIndex = sockIndex;
										r->address = adr;
									}
								}
								else
								{
									auto ac = accepting.lock();
									if (!ac)
									{
										UDP_LOG(7, "received invalid packet (unknown connection id)");
										continue;
									}
									auto s = std::make_shared<Receiver>();
									s->address = adr;
									s->connId = connId;
									s->packets.push_back(mv);
									s->sockIndex = sockIndex;
									receivers[connId] = s;
									ac->push_back(s);
								}
							}
						}
					}
					catch (...)
					{
						// nothing?
					}
				}
				for (auto it = receivers.begin(); it != receivers.end();)
				{
					if (!it->second.lock())
						it = receivers.erase(it);
					else
						it++;
				}
			}
		};

		// compare sequence numbers with correct wrapping
		// semantically: return a < b
		constexpr bool comp(uint16 a, uint16 b)
		{
			return (a < b && b - a < 32768) || (a > b && a - b > 32768);
		}

		static_assert(comp(5, 10));
		static_assert(comp(50000, 60000));
		static_assert(comp(60000, 5));
		static_assert(!comp(10, 5));
		static_assert(!comp(60000, 50000));
		static_assert(!comp(5, 60000));
		static_assert(!comp(5, 5));
		static_assert(!comp(60000, 60000));

		constexpr FlatSet<uint16> decodeAck(uint16 seqn, uint32 bits)
		{
			FlatSet<uint16> result;
			result.reserve(32);
			for (uint16 i = 0; i < 32; i++)
			{
				const uint32 m = uint32(1) << i;
				if ((bits & m) == m)
				{
					uint16 s = seqn - i;
					result.unsafeData().push_back(s);
				}
			}
			std::sort(result.unsafeData().begin(), result.unsafeData().end());
			return result;
		}

		constexpr uint32 encodeAck(uint16 seqn, const FlatSet<uint16> &bits)
		{
			uint32 result = 0;
			for (uint16 i = 0; i < 32; i++)
			{
				uint16 s = seqn - i;
				if (bits.count(s))
				{
					uint32 m = uint32(1) << i;
					result |= m;
				}
			}
			return result;
		}

		static_assert(decodeAck(1000, encodeAck(1000, { 999 })) == FlatSet<uint16>({ 999 }));
		static_assert(decodeAck(1000, encodeAck(1000, { 1000 })) == FlatSet<uint16>({ 1000 }));
		static_assert(decodeAck(1000, encodeAck(1000, { 1000, 999 })) == FlatSet<uint16>({ 1000, 999 }));
		static_assert(decodeAck(1000, encodeAck(1000, { 1000, 999, 990 })) == FlatSet<uint16>({ 1000, 999, 990 }));
		static_assert(decodeAck(5, encodeAck(5, { 1, 65533 })) == FlatSet<uint16>({ 1, 65533 }));

		struct PackAck
		{
			uint32 ackBits = 0;
			uint16 ackSeqn = 0;

			PackAck() = default;

			PackAck(uint16 seqn, uint32 bits) : ackBits(bits), ackSeqn(seqn) {}

			bool operator<(const PackAck &other) const
			{
				if (ackSeqn == other.ackSeqn)
					return ackBits < other.ackBits;
				return ackSeqn < other.ackSeqn;
			}
		};

		enum class CmdTypeEnum : uint8
		{
			Invalid = 0,
			ConnectionInit = 1, // uint16 responseIndex, uint16 LongSize
			ConnectionFinish = 2,
			Acknowledgement = 13, // uint16 packetSeqn, uint32 bits
			ShortMessage = 20, // uint8 channel, uint16 msgSeqn, uint16 size, data
			LongMessage = 21, // uint8 channel, uint16 msgSeqn, uint32 totalSize, uint16 index, data
			StatsDiscovery = 42, // uint64 aReceivedBytes, uint64 aSentBytes, uint64 aTime, uint64 bReceivedBytes, uint64 bSentBytes, uint64 bTime, uint32 aReceivedPackets, uint32 aSentPackets, uint32 bReceivedPackets, uint32 bSentPackets, uint16 step
			MtuDiscovery = 43, // todo
		};

		constexpr uint16 LongSize = 470; // designed to work well with default mtu (fits 3 LongMessage commands in single packet)

		constexpr uint16 longCmdsCount(uint32 totalSize)
		{
			return (totalSize + LongSize - 1) / LongSize;
		}

		class GinnelConnectionImpl : public GinnelConnection
		{
		public:
			GinnelConnectionImpl(const String &address, uint16 port, uint64 timeout) : connId(randomRange((uint64)1, (uint64)m - 1))
			{
				UDP_LOG(1, "creating new connection to address: '" + address + "', port: " + port + ", timeout: " + timeout);
				detail::OverrideBreakpoint ob;
				sockGroup = std::make_shared<SockGroup>();
				for (AddrList lst(address, port, AF_UNSPEC, SOCK_DGRAM, IPPROTO_UDP, 0); lst.valid(); lst.next())
				{
					try
					{
						Sock s = lst.sock();
						s.setBlocking(false);
						s.connect(lst.address());
						sockGroup->socks.push_back(std::move(s));
					}
					catch (...)
					{
						// nothing
					}
				}
				initializationCompletion(timeout);
			}

			GinnelConnectionImpl(const String &localAddress, uint16 localPort, const String &remoteAddress, uint16 remotePort, uint64 connId, uint64 timeout) : connId(connId)
			{
				CAGE_ASSERT(connId != 0 && connId != m);
				UDP_LOG(1, "creating new connection to remote address: '" + remoteAddress + "', remote port: " + remotePort + ", at local address: '" + localAddress + "', local port: '" + localPort + "', timeout: " + timeout);
				detail::OverrideBreakpoint ob;
				sockGroup = std::make_shared<SockGroup>();
				for (AddrList ll(localAddress, localPort, AF_UNSPEC, SOCK_DGRAM, IPPROTO_UDP, AI_PASSIVE); ll.valid(); ll.next())
				{
					for (AddrList rl(remoteAddress, remotePort, AF_UNSPEC, SOCK_DGRAM, IPPROTO_UDP, 0); rl.valid(); rl.next())
					{
						if (ll.family() != rl.family())
							continue;
						try
						{
							Sock s = ll.sock();
							s.setBlocking(false);
							s.bind(ll.address());
							s.connect(rl.address());
							sockGroup->socks.push_back(std::move(s));
						}
						catch (...)
						{
							// nothing
						}
					}
				}
				initializationCompletion(timeout);
			}

			GinnelConnectionImpl(Sock &&sock, uint64 connId, uint64 timeout) : connId(connId)
			{
				CAGE_ASSERT(connId != 0 && connId != m);
				CAGE_ASSERT(sock.isValid());
				CAGE_ASSERT(sock.getType() == SOCK_DGRAM);
				CAGE_ASSERT(sock.getProtocol() == IPPROTO_UDP);
				CAGE_ASSERT(sock.getConnected());
				UDP_LOG(1, "creating new connection with given socket");
				sock.setBlocking(false);
				sockGroup = std::make_shared<SockGroup>();
				sockGroup->socks.push_back(std::move(sock));
				initializationCompletion(timeout);
			}

			void initializationCompletion(uint64 timeout)
			{
				if (sockGroup->socks.empty())
					CAGE_THROW_ERROR(Exception, "failed to connect (no sockets available)");
				UDP_LOG(2, "created " + sockGroup->socks.size() + " sockets");
				sockGroup->applyBufferSizes();

				sockReceiver = std::make_shared<SockGroup::Receiver>();
				sockReceiver->connId = connId;
				sockGroup->receivers[connId] = sockReceiver;

				// initialize connection
				service();
				if (timeout)
				{
					while (true)
					{
						threadSleep(5000);
						service();
						if (established)
							break;
						if (applicationTime() > startTime + timeout)
							CAGE_THROW_ERROR(Disconnected, "failed to connect (timeout)");
					}
				}
			}

			GinnelConnectionImpl(std::shared_ptr<SockGroup> sg, std::shared_ptr<SockGroup::Receiver> rec) : sockGroup(sg), sockReceiver(rec), connId(rec->connId)
			{
				CAGE_ASSERT(rec->sockIndex < sg->socks.size());
				CAGE_ASSERT(!sg->socks[rec->sockIndex].getConnected());
				UDP_LOG(1, "accepting new connection");
			}

			~GinnelConnectionImpl()
			{
				UDP_LOG(2, "destroying connection");

				// send connection closed packet
				if (established)
				{
					detail::OverrideBreakpoint ob;
					try
					{
						sending.cmds.clear();
						Sending::Command cmd;
						cmd.type = CmdTypeEnum::ConnectionFinish;
						sending.cmds.push_back(cmd);
						composePackets();
					}
					catch (...)
					{
						// nothing
					}
				}
			}

			// SENDING

			void dispatchPacket(const void *data, uintPtr size)
			{
				stats.bytesSentTotal += size;
				stats.packetsSentTotal++;

				{ // simulated packet loss for testing purposes
					const float ch = confSimulatedPacketLoss;
					if (ch > 0 && randomChance() < ch)
					{
						UDP_LOG(4, "dropping packet due to simulated packet loss");
						return;
					}
				}

				// sending does not need to be under mutex
				if (sockReceiver->sockIndex == m)
				{
					// client-side or p2p connection
					for (Sock &s : sockGroup->socks)
					{
						if (!s.isValid())
							continue;
						CAGE_ASSERT(s.getConnected());
						s.send(data, size);
					}
				}
				else
				{
					// server-side of a connection
					Sock &s = sockGroup->socks[sockReceiver->sockIndex];
					if (s.isValid())
					{
						if (s.getConnected())
							s.send(data, size);
						else
							s.sendTo(data, size, sockReceiver->address);
					}
				}
			}

			struct Sending
			{
				struct ReliableMsg
				{
					Holder<MemoryBuffer> data;
					std::vector<bool> parts; // acked parts
					uint16 step = 0;
					uint16 msgSeqn = 0;
					uint8 channel = 0;
				};

				struct MsgAck
				{
					std::weak_ptr<ReliableMsg> msg;
					uint16 index = 0;
				};

				union CommandUnion
				{
					uint16 initIndex = 0;

					PackAck ack;

					struct Msg
					{
						uint16 msgSeqn = 0;
						uint8 channel = 0;
					} msg;

					struct Stats
					{
						struct SideStruct
						{
							uint64 time = 0;
							uint64 receivedBytes = 0;
							uint64 sentBytes = 0;
							uint32 receivedPackets = 0;
							uint32 sentPackets = 0;
						} a, b;
						uint16 step = 0;
					} stats;

					CommandUnion() {}
				};

				struct Command
				{
					CommandUnion data;
					MsgAck msgAck;
					MemView msgData;
					CmdTypeEnum type = CmdTypeEnum::Invalid;
					sint8 priority = 0;
				};

				plf::list<std::shared_ptr<ReliableMsg>> relMsgs;
				plf::list<Command> cmds;
				ankerl::unordered_dense::map<uint16, std::vector<MsgAck>> ackMap; // mapping packet seqn to message parts
				std::array<uint16, 256> seqnPerChannel = {}; // next message seqn to be used
				FlatSet<uint16> seqnToAck; // packets seqn to be acked
				uint16 packetSeqn = 0; // next packet seqn to be used
			} sending;

			void generateCommands(std::shared_ptr<Sending::ReliableMsg> &msg, sint8 priority)
			{
				Sending::Command cmd;
				cmd.msgData.buffer = msg->data.share();
				cmd.data.msg.msgSeqn = msg->msgSeqn;
				cmd.data.msg.channel = msg->channel;
				cmd.priority = priority;
				if (msg->channel >= 128)
					cmd.msgAck.msg = msg;

				bool completelyAcked = true;

				if (msg->data->size() > LongSize)
				{ // long message
					cmd.type = CmdTypeEnum::LongMessage;
					uint16 totalCount = longCmdsCount(numeric_cast<uint32>(msg->data->size()));
					for (uint16 index = 0; index < totalCount; index++)
					{
						if (msg->parts.empty() || !msg->parts[index])
						{
							cmd.msgData.offset = index * LongSize;
							cmd.msgData.size = index + 1 == totalCount ? msg->data->size() - cmd.msgData.offset : LongSize;
							cmd.msgAck.index = index;
							sending.cmds.push_back(cmd);
							completelyAcked = false;
						}
					}
				}
				else
				{ // short message
					if (msg->parts.empty() || !msg->parts[0])
					{
						cmd.msgData.size = msg->data->size();
						cmd.type = CmdTypeEnum::ShortMessage;
						sending.cmds.push_back(cmd);
						completelyAcked = false;
					}
				}

				if (completelyAcked)
					msg.reset();
			}

			void generateAckCommands(sint8 priority)
			{
				if (sending.seqnToAck.empty())
					return;

				std::vector<PackAck> acks;
				auto it = sending.seqnToAck.rbegin();
				auto et = sending.seqnToAck.rend();
				uint16 front = *it++;
				FlatSet<uint16> tmp;
				tmp.insert(front);
				while (it != et)
				{
					uint16 n = *it++;
					uint16 dist = front - n;
					if (dist < 32)
						tmp.insert(n);
					else
					{
						PackAck p(front, encodeAck(front, tmp));
						CAGE_ASSERT(decodeAck(p.ackSeqn, p.ackBits) == tmp);
						acks.push_back(p);
						tmp.clear();
						front = n;
						tmp.insert(front);
					}
				}
				{
					PackAck p(front, encodeAck(front, tmp));
					CAGE_ASSERT(decodeAck(p.ackSeqn, p.ackBits) == tmp);
					acks.push_back(p);
				}

#ifdef CAGE_ASSERT_ENABLED
				{ // verification
					FlatSet<uint16> ver;
					for (PackAck pa : acks)
					{
						FlatSet<uint16> c = decodeAck(pa.ackSeqn, pa.ackBits);
						ver.insert(c.begin(), c.end());
					}
					CAGE_ASSERT(ver == sending.seqnToAck);
				}
#endif // CAGE_ASSERT_ENABLED

				for (const PackAck &pa : acks)
				{
					Sending::Command cmd;
					cmd.data.ack = pa;
					cmd.type = CmdTypeEnum::Acknowledgement;
					cmd.priority = priority;
					sending.cmds.push_back(std::move(cmd));
				}
			}

			void resendReliableMessages()
			{
				for (std::shared_ptr<Sending::ReliableMsg> &msg : sending.relMsgs)
				{
					if (!msg)
						continue;
					sint8 priority = -1;
					switch (msg->step)
					{
						case 0:
							// purposefully skip number one, it is almost impossible to get ack that fast
						case 2:
						case 4:
							priority = 0;
							break;
						case 8:
						case 16:
							priority = 1;
							break;
						case 30:
						case 60:
						case 90:
							priority = 2;
							break;
						case 120:
						case 150:
						case 180:
							priority = 3;
							break;
						case 210:
						case 240:
						case 270:
							priority = 4;
							break;
					}
					if (msg && msg->step++ >= 300)
						CAGE_THROW_ERROR(Disconnected, "too many failed attempts at sending a reliable message");
					if (priority >= 0)
						generateCommands(msg, priority);
				}
			}

			void serializeCommand(const Sending::Command &cmd, Serializer &ser)
			{
				ser << cmd.type;
				switch (cmd.type)
				{
					case CmdTypeEnum::ConnectionInit:
					{
						ser << cmd.data.initIndex << LongSize;
						break;
					}
					case CmdTypeEnum::ConnectionFinish:
					{
						// nothing
						break;
					}
					case CmdTypeEnum::Acknowledgement:
					{
						ser << cmd.data.ack.ackSeqn << cmd.data.ack.ackBits;
						break;
					}
					case CmdTypeEnum::ShortMessage:
					{
						ser << cmd.data.msg.channel << cmd.data.msg.msgSeqn;
						uint16 size = numeric_cast<uint16>(cmd.msgData.size);
						ser << size;
						ser.write({ cmd.msgData.buffer->data(), cmd.msgData.buffer->data() + cmd.msgData.size });
						break;
					}
					case CmdTypeEnum::LongMessage:
					{
						ser << cmd.data.msg.channel << cmd.data.msg.msgSeqn;
						uint32 totalSize = numeric_cast<uint32>(cmd.msgData.buffer->size());
						uint16 index = numeric_cast<uint16>(cmd.msgData.offset / LongSize);
						ser << totalSize << index;
						ser.write({ cmd.msgData.buffer->data() + cmd.msgData.offset, cmd.msgData.buffer->data() + cmd.msgData.offset + cmd.msgData.size });
						break;
					}
					case CmdTypeEnum::StatsDiscovery:
					{
						const auto &s = cmd.data.stats;
						ser << s.a.receivedBytes << s.a.sentBytes << s.a.time << s.b.receivedBytes << s.b.sentBytes << s.b.time << s.a.receivedPackets << s.a.sentPackets << s.b.receivedPackets << s.b.sentPackets << s.step;
						break;
					}
					default:
						CAGE_THROW_CRITICAL(Exception, "invalid udp command type enum");
				}
			}

			void composePackets()
			{
				static constexpr uint32 mtu = 1450;
				MemoryBuffer buff;
				buff.reserve(mtu);
				Serializer ser(buff);
				uint16 currentPacketSeqn = 0;
				bool empty = true;
				for (const Sending::Command &cmd : sending.cmds)
				{
					const uint32 cmdSize = numeric_cast<uint32>(cmd.msgData.size) + 10;

					// send current packet
					if (!empty && buff.size() + cmdSize > mtu)
					{
						dispatchPacket(buff.data(), buff.size());
						buff.resize(0);
						ser = Serializer(buff);
						//empty = true;
					}

					// generate packet header
					if (buff.size() == 0)
					{
						currentPacketSeqn = sending.packetSeqn++;
						ser << CageMagic << connId << currentPacketSeqn;
					}

					// serialize the command
					serializeCommand(cmd, ser);
					if (cmd.msgAck.msg.lock())
						sending.ackMap[currentPacketSeqn].push_back(cmd.msgAck);
					empty = false;
				}
				if (!empty)
					dispatchPacket(buff.data(), buff.size());
			}

			void serviceSending()
			{
				if (!established)
				{
					Sending::Command cmd;
					cmd.type = CmdTypeEnum::ConnectionInit;
					cmd.priority = 10;
					sending.cmds.push_back(cmd);
				}

				// send new stats
				if (currentServiceTime > lastStatsSendTime + 100000)
				{
					Sending::CommandUnion::Stats p;
					handleStats(p);
				}

				generateAckCommands(0);
				resendReliableMessages();

				sending.cmds.sort(
					[](const Sending::Command &a, const Sending::Command &b)
					{
						return a.priority > b.priority; // higher priority first
					});

				try
				{
					composePackets();
				}
				catch (const cage::Exception &)
				{
					// do nothing
				}

				// finish up
				sending.cmds.clear();
				generateAckCommands(3); // duplicate current acks in next iteration
				sending.seqnToAck.clear();

				{ // clear finished reliable messages
					std::erase_if(sending.relMsgs, [](const std::shared_ptr<Sending::ReliableMsg> &v) -> bool { return !v; });
				}

				{ // clear ackMap
					std::erase_if(sending.ackMap,
						[](std::pair<uint16, std::vector<Sending::MsgAck>> &v) -> bool
						{
							std::erase_if(v.second, [](Sending::MsgAck &p) { return !p.msg.lock(); });
							return v.second.empty();
						});
				}
			}

			// RECEIVING

			struct Receiving
			{
				struct Msg
				{
					MemoryBuffer data;
					std::vector<bool> parts; // acked parts
					uint16 msgSeqn = 0;
					uint8 channel = 0;
				};

				std::array<ankerl::unordered_dense::map<uint16, Msg>, 256> staging = {};
				std::array<uint16, 256> seqnPerChannel = {}; // minimum expected message seqn
				plf::list<Msg> messages;
				FlatSet<PackAck> ackPacks;
				uint16 packetSeqn = 0; // minimum expected packet seqn
			} receiving;

			void processAcks()
			{
				FlatSet<uint16> acks;
				for (const auto &a : receiving.ackPacks)
				{
					FlatSet<uint16> s = decodeAck(a.ackSeqn, a.ackBits);
					acks.insert(s.begin(), s.end());
				}
				receiving.ackPacks.clear();
				for (uint16 ack : acks)
				{
					if (sending.ackMap.count(ack) == 0)
						continue;
					for (const auto &ma : sending.ackMap[ack])
					{
						if (const auto m = ma.msg.lock())
							m->parts[ma.index] = true;
					}
					sending.ackMap.erase(ack);
				}
			}

			void processReceived()
			{
				// unreliable
				for (uint32 channel = 0; channel < 128; channel++)
				{
					auto &seqnpc = receiving.seqnPerChannel[channel];
					auto &stage = receiving.staging[channel];
					std::vector<Receiving::Msg *> msgs;
					msgs.reserve(stage.size());
					for (auto &it : stage)
						msgs.push_back(&it.second);
					std::sort(msgs.begin(), msgs.end(), [](const Receiving::Msg *a, Receiving::Msg *b) { return comp(a->msgSeqn, b->msgSeqn); });
					for (Receiving::Msg *msg : msgs)
					{
						if (msg->data.size() == 0)
							continue;
						if (comp(msg->msgSeqn, seqnpc))
						{
							// this message is obsolete
							msg->data.free();
							continue;
						}
						bool complete = true;
						for (bool p : msg->parts)
							complete = complete && p;
						if (!complete)
							continue;
						seqnpc = msg->msgSeqn + (uint16)1;
						receiving.messages.push_back(std::move(*msg));
					}
				}

				// reliable
				for (uint32 channel = 128; channel < 256; channel++)
				{
					auto &seqnpc = receiving.seqnPerChannel[channel];
					auto &stage = receiving.staging[channel];
					while (true)
					{
						if (stage.count(seqnpc) == 0)
							break;
						Receiving::Msg &msg = stage[seqnpc];
						if (msg.data.size() == 0)
							break;
						bool complete = true;
						for (bool p : msg.parts)
							complete = complete && p;
						if (!complete)
							break;
						seqnpc++;
						receiving.messages.push_back(std::move(msg));
					}
				}

				// erase empty
				for (auto &stage : receiving.staging)
				{
					std::erase_if(stage, [](const std::pair<uint16, Receiving::Msg> &v) -> bool { return v.second.data.size() == 0; });
				}
			}

			void handleStats(Sending::CommandUnion::Stats &p)
			{
				GinnelStatistics &s = stats;
				if (p.step > 1)
				{
					if (p.b.time <= s.timestamp)
						return; // discard obsolete stats packet
					s.timestamp = p.b.time;
					s.roundTripDuration = currentServiceTime - p.b.time;
					s.estimatedBandwidth = writeBandwidth.bandwidth;
					s.bytesReceivedLately = s.bytesReceivedTotal - p.b.receivedBytes;
					s.bytesSentLately = s.bytesSentTotal - p.b.sentBytes;
					s.bytesDeliveredLately = p.a.receivedBytes - s.bytesDeliveredTotal;
					s.bytesDeliveredTotal = p.a.receivedBytes;
					s.packetsReceivedLately = s.packetsReceivedTotal - p.b.receivedPackets;
					s.packetsSentLately = s.packetsSentTotal - p.b.sentPackets;
					s.packetsDeliveredLately = p.a.receivedPackets - s.packetsDeliveredTotal;
					s.packetsDeliveredTotal = p.a.receivedPackets;
				}
				p.b = p.a;
				p.a.receivedBytes = s.bytesReceivedTotal;
				p.a.receivedPackets = s.packetsReceivedTotal;
				p.a.sentBytes = s.bytesSentTotal;
				p.a.sentPackets = s.packetsSentTotal;
				p.a.time = currentServiceTime;
				p.step++;
				Sending::Command cmd;
				cmd.data.stats = p;
				cmd.type = CmdTypeEnum::StatsDiscovery;
				cmd.priority = 3;
				sending.cmds.push_back(std::move(cmd));
				lastStatsSendTime = currentServiceTime;
			}

			void handleReceivedCommand(Deserializer &d)
			{
				CmdTypeEnum type;
				d >> type;
				switch (type)
				{
					case CmdTypeEnum::ConnectionInit:
					{
						uint16 index = 0;
						uint16 longSize = 0;
						d >> index >> longSize;
						UDP_LOG(3, "received connection init command with index " + index);
						if (longSize != LongSize)
						{
							UDP_LOG(3, "the connection has incompatible LongSize: " + longSize + ", my LongSize: " + LongSize);
							CAGE_THROW_ERROR(Disconnected, "incompatible connection (LongSize)");
						}
						if (!established)
						{
							if (logLevel >= 2)
							{
								const auto n1 = sockReceiver->address.translate(false);
								const auto n2 = sockReceiver->address.translate(true);
								CAGE_LOG(SeverityEnum::Info, "udp", Stringizer() + "connection established, remote address: " + n1.first + ", remote name: " + n2.first + ", remote port: " + n1.second);
							}
							established = true;
						}
						if (index == 0)
						{
							Sending::Command cmd;
							cmd.data.initIndex = index + 1;
							cmd.type = CmdTypeEnum::ConnectionInit;
							cmd.priority = 10;
							sending.cmds.push_back(cmd);
						}
						break;
					}
					case CmdTypeEnum::ConnectionFinish:
					{
						CAGE_THROW_ERROR(Disconnected, "connection closed by peer");
						break;
					}
					case CmdTypeEnum::Acknowledgement:
					{
						PackAck ackPack;
						d >> ackPack.ackSeqn >> ackPack.ackBits;
						receiving.ackPacks.insert(ackPack);
						break;
					}
					case CmdTypeEnum::ShortMessage:
					{
						Receiving::Msg msg;
						uint16 size = 0;
						d >> msg.channel >> msg.msgSeqn >> size;
						if (comp(msg.msgSeqn, receiving.seqnPerChannel[msg.channel]))
						{ // obsolete
							d.read(size);
						}
						else
						{
							msg.data.resize(size);
							d.read(msg.data);
							receiving.staging[msg.channel][msg.msgSeqn] = std::move(msg);
						}
						break;
					}
					case CmdTypeEnum::LongMessage:
					{
						uint8 channel = 0;
						uint16 msgSeqn = 0;
						uint32 totalSize = 0;
						uint16 index = 0;
						d >> channel >> msgSeqn >> totalSize >> index;
						uint16 totalCount = longCmdsCount(totalSize);
						if (index >= totalCount)
							CAGE_THROW_ERROR(Exception, "invalid long message index");
						uint16 size = index + 1 == totalCount ? totalSize - (totalCount - 1) * LongSize : LongSize;
						if (comp(msgSeqn, receiving.seqnPerChannel[channel]))
						{ // obsolete
							d.read(size);
						}
						else
						{
							Receiving::Msg &msg = receiving.staging[channel][msgSeqn];
							if (msg.data.size() == 0)
							{
								msg.data.resize(totalSize);
								msg.channel = channel;
								msg.msgSeqn = msgSeqn;
								msg.parts.resize(totalCount);
							}
							else if (msg.data.size() != totalSize)
								CAGE_THROW_ERROR(Exception, "inconsistent message total size");
							msg.parts[index] = true;
							d.read({ msg.data.data() + index * LongSize, msg.data.data() + index * LongSize + size });
						}
						break;
					}
					case CmdTypeEnum::StatsDiscovery:
					{
						Sending::CommandUnion::Stats s;
						d >> s.a.receivedBytes >> s.a.sentBytes >> s.a.time >> s.b.receivedBytes >> s.b.sentBytes >> s.b.time >> s.a.receivedPackets >> s.a.sentPackets >> s.b.receivedPackets >> s.b.sentPackets >> s.step;
						handleStats(s);
						break;
					}
					default:
						CAGE_THROW_ERROR(Exception, "invalid message type enum");
				}
			}

			void handleReceivedPacket(const MemView &b)
			{
				Deserializer d = b.des();
				UDP_LOG(5, "received packet with " + d.available() + " bytes");
				stats.bytesReceivedTotal += d.available();
				stats.packetsReceivedTotal++;
				{ // read signature and connection id
					uint32 sign;
					uint64 id;
					d >> sign >> id;
					CAGE_ASSERT(sign == CageMagic && id == connId);
				}
				{ // read packet header
					uint16 packetSeqn;
					d >> packetSeqn;
					UDP_LOG(3, "received packet seqn " + packetSeqn);
					// check if the packet is in order
					if (comp(packetSeqn, receiving.packetSeqn))
					{
						UDP_LOG(4, "the packet is out of order");
						// the packet is a duplicate or was delayed
						// since packets duplication or delays are common on network, we do not want to close the socket
						//   therefore we ignore the packet, but do not throw an exception
						return;
					}
					receiving.packetSeqn = packetSeqn + (uint16)1;
					sending.seqnToAck.insert(packetSeqn);
				}
				// read packet commands
				while (d.available() > 0)
					handleReceivedCommand(d);
			}

			void serviceReceiving()
			{
				std::vector<MemView> packets;
				{
					ScopeLock lock(sockGroup->mut);
					sockGroup->readAll();
					sockReceiver->packets.swap(packets);
				}
				try
				{
					for (MemView &b : packets)
						handleReceivedPacket(b);
				}
				catch (const Disconnected &)
				{
					throw;
				}
				catch (cage::Exception &)
				{
					// do nothing
				}
				processAcks();
				processReceived();
			}

			// WRITE BANDWIDTH

			struct WriteBandwidth
			{
				uint64 statsTimestamp = 0;
				uint64 bandwidth = 50 * 1024; // start at 50 KBps
				sint64 capacity = 5 * 1024;
				sint32 quality = 0;
			} writeBandwidth;

			void serviceWriteBandwidth()
			{
				WriteBandwidth &wb = writeBandwidth;

				// update quality
				if (currentServiceTime > stats.timestamp + 100 * deltaTime)
				{
					wb.quality--;
				}
				else if (stats.timestamp != wb.statsTimestamp && stats.roundTripDuration > 0)
				{
					wb.statsTimestamp = stats.timestamp;
					if (100 * stats.packetsDeliveredLately < 95 * stats.packetsSentLately && stats.packetsSentLately > 5)
						wb.quality--;
					else if (100 * stats.bpsDelivered() > 80 * wb.bandwidth)
						wb.quality++;
				}

				// update bandwidth
				if (wb.quality < -2)
				{
					wb.bandwidth = max(70 * wb.bandwidth / 100, uint64(10000));
					wb.quality = 0;
				}
				else if (wb.quality > 2)
				{
					wb.bandwidth = 115 * wb.bandwidth / 100;
					wb.quality = 0;
				}

				// allow the capacity to accumulate over multiple updates
				// but restrict the capacity in some reasonable range
				wb.capacity = clamp(wb.capacity + deltaTime * wb.bandwidth / 1000000, uint64(0), 10 * deltaTime * wb.bandwidth / 1000000);
			}

			// COMMON

			GinnelStatistics stats;
			std::shared_ptr<SockGroup> sockGroup;
			std::shared_ptr<SockGroup::Receiver> sockReceiver;
			const uint64 startTime = applicationTime();
			const uint64 connId = m;
			uint64 lastStatsSendTime = 0;
			uint64 currentServiceTime = 0; // time at which this service has started
			uint64 deltaTime = 0; // time elapsed since last service
			bool established = false;

			// API

			Holder<PointerRange<char>> read(uint32 &channel, bool &reliable)
			{
				if (receiving.messages.empty())
					return {};
				auto tmp = std::move(receiving.messages.front());
				receiving.messages.pop_front();
				channel = tmp.channel & 127; // strip reliability bit
				reliable = tmp.channel > 127;
				return std::move(tmp.data);
			}

			void write(MemoryBuffer &&buffer, uint32 channel, bool reliable)
			{
				CAGE_ASSERT(channel < 128);
				CAGE_ASSERT(buffer.size() <= 16 * 1024 * 1024);
				if (buffer.size() == 0)
					return; // ignore empty messages

				writeBandwidth.capacity -= buffer.size();
				auto msg = std::make_shared<Sending::ReliableMsg>();
				msg->data = systemMemory().createHolder<MemoryBuffer>(std::move(buffer));
				msg->channel = numeric_cast<uint8>(channel + reliable * 128);
				msg->msgSeqn = sending.seqnPerChannel[msg->channel]++;
				if (reliable)
				{
					msg->parts.resize(longCmdsCount(numeric_cast<uint32>(msg->data->size())));
					sending.relMsgs.push_back(std::move(msg));
				}
				else
					generateCommands(msg, 0);
			}

			void service()
			{
				const uint64 newTime = applicationTime();
				deltaTime = newTime - currentServiceTime;
				currentServiceTime = newTime;
				detail::OverrideBreakpoint brk;
				serviceReceiving();
				serviceSending();
				serviceWriteBandwidth();
			}
		};

		class GinnelServerImpl : public GinnelServer
		{
		public:
			GinnelServerImpl(uint16 port)
			{
				UDP_LOG(1, "creating new server on port " + port);
				detail::OverrideBreakpoint ob;
				sockGroup = std::make_shared<SockGroup>();
				for (AddrList lst(nullptr, port, AF_UNSPEC, SOCK_DGRAM, IPPROTO_UDP, AI_PASSIVE); lst.valid(); lst.next())
				{
					try
					{
						Sock s = lst.sock();
						s.setBlocking(false);
						s.setReuseaddr(true);
						s.bind(lst.address());
						sockGroup->socks.push_back(std::move(s));
					}
					catch (...)
					{
						// nothing
					}
				}
				if (sockGroup->socks.empty())
					CAGE_THROW_ERROR(Exception, "failed to bind (no sockets available)");
				sockGroup->applyBufferSizes();
				accepting = std::make_shared<std::vector<std::shared_ptr<SockGroup::Receiver>>>();
				sockGroup->accepting = accepting;
				UDP_LOG(2, "listening on " + sockGroup->socks.size() + " sockets");
			}

			~GinnelServerImpl() { UDP_LOG(2, "destroying server"); }

			Holder<GinnelConnection> accept()
			{
				detail::OverrideBreakpoint brk;
				std::shared_ptr<SockGroup::Receiver> acc;
				{
					ScopeLock lock(sockGroup->mut);
					sockGroup->readAll();
					if (accepting->empty())
						return {};
					acc = accepting->back();
					accepting->pop_back();
				}
				try
				{
					auto c = systemMemory().createHolder<GinnelConnectionImpl>(sockGroup, acc);
					c->serviceReceiving();
					if (!c->established)
					{
						UDP_LOG(2, "received packets failed to initialize new connection");
						return {};
					}
					return std::move(c).cast<GinnelConnection>();
				}
				catch (...)
				{
					return {};
				}
			}

			std::shared_ptr<SockGroup> sockGroup;
			std::shared_ptr<std::vector<std::shared_ptr<SockGroup::Receiver>>> accepting;
		};
	}

	uint64 GinnelStatistics::bpsReceived() const
	{
		if (roundTripDuration)
			return 1000000 * bytesReceivedLately / roundTripDuration;
		return 0;
	}

	uint64 GinnelStatistics::bpsSent() const
	{
		if (roundTripDuration)
			return 1000000 * bytesSentLately / roundTripDuration;
		return 0;
	}

	uint64 GinnelStatistics::bpsDelivered() const
	{
		if (roundTripDuration)
			return 1000000 * bytesDeliveredLately / roundTripDuration;
		return 0;
	}

	uint64 GinnelStatistics::ppsReceived() const
	{
		if (roundTripDuration)
			return uint64(1000000) * packetsReceivedLately / roundTripDuration;
		return 0;
	}

	uint64 GinnelStatistics::ppsSent() const
	{
		if (roundTripDuration)
			return uint64(1000000) * packetsSentLately / roundTripDuration;
		return 0;
	}

	uint64 GinnelStatistics::ppsDelivered() const
	{
		if (roundTripDuration)
			return uint64(1000000) * packetsDeliveredLately / roundTripDuration;
		return 0;
	}

	Holder<PointerRange<char>> GinnelConnection::read(uint32 &channel, bool &reliable)
	{
		GinnelConnectionImpl *impl = (GinnelConnectionImpl *)this;
		return impl->read(channel, reliable);
	}

	Holder<PointerRange<char>> GinnelConnection::read()
	{
		uint32 channel;
		bool reliable;
		return read(channel, reliable);
	}

	void GinnelConnection::write(PointerRange<const char> buffer, uint32 channel, bool reliable)
	{
		GinnelConnectionImpl *impl = (GinnelConnectionImpl *)this;
		MemoryBuffer b(buffer.size());
		detail::memcpy(b.data(), buffer.data(), b.size());
		impl->write(std::move(b), channel, reliable);
	}

	sint64 GinnelConnection::capacity() const
	{
		const GinnelConnectionImpl *impl = (const GinnelConnectionImpl *)this;
		return impl->writeBandwidth.capacity;
	}

	void GinnelConnection::update()
	{
		GinnelConnectionImpl *impl = (GinnelConnectionImpl *)this;
		impl->service();
	}

	const GinnelStatistics &GinnelConnection::statistics() const
	{
		const GinnelConnectionImpl *impl = (const GinnelConnectionImpl *)this;
		return impl->stats;
	}

	bool GinnelConnection::established() const
	{
		const GinnelConnectionImpl *impl = (const GinnelConnectionImpl *)this;
		return impl->established;
	}

	GinnelRemoteInfo GinnelConnection::remoteInfo() const
	{
		const GinnelConnectionImpl *impl = (const GinnelConnectionImpl *)this;
		if (!impl->sockGroup || impl->sockGroup->socks.empty())
			return {};
		const auto a = impl->sockGroup->socks[0].getRemoteAddress().translate(false);
		return { a.first, a.second };
	}

	uint16 GinnelServer::port() const
	{
		const GinnelServerImpl *impl = (const GinnelServerImpl *)this;
		if (!impl->sockGroup || impl->sockGroup->socks.empty())
			return 0;
		return impl->sockGroup->socks[0].getLocalAddress().getPort();
	}

	Holder<GinnelConnection> GinnelServer::accept()
	{
		GinnelServerImpl *impl = (GinnelServerImpl *)this;
		return impl->accept();
	}

	Holder<GinnelConnection> newGinnelConnection(const String &address, uint16 port, uint64 timeout)
	{
#ifdef CAGE_SYSTEM_MAC
		CAGE_LOG(SeverityEnum::Warning, "ginnel", "ginnel on macos might be broken - it is excluded from tests");
#endif // CAGE_SYSTEM_MAC
		return systemMemory().createImpl<GinnelConnection, GinnelConnectionImpl>(address, port, timeout);
	}

	Holder<GinnelServer> newGinnelServer(uint16 port)
	{
#ifdef CAGE_SYSTEM_MAC
		CAGE_LOG(SeverityEnum::Warning, "ginnel", "ginnel on macos might be broken - it is excluded from tests");
#endif // CAGE_SYSTEM_MAC
		return systemMemory().createImpl<GinnelServer, GinnelServerImpl>(port);
	}
}
