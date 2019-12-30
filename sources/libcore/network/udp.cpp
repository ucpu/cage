#include "net.h"
#include <cage-core/math.h> // random
#include <cage-core/config.h>
#include <cage-core/concurrent.h>
#include <cage-core/serialization.h>

#include <array>
#include <vector>
#include <set>
#include <map>
#include <list>
#include <memory>
#include <algorithm>

namespace cage
{
	// nomenclature:
	// message - UdpConnection methods read and write are working with messages
	// packet - system functions work with packets
	// command - messages are fragmented into commands which are grouped into packets

	namespace
	{
		using namespace privat;

		ConfigUint32 logLevel("cage/udp/logLevel", 0);
		ConfigFloat confSimulatedPacketLoss("cage/udp/simulatedPacketLoss", 0);
		ConfigUint32 confMtu("cage/udp/maxTransferUnit", 1450);
		ConfigUint32 confBufferSize("cage/udp/systemBufferSize", 1024 * 1024);

#define UDP_LOG(LEVEL, MSG) { if (logLevel >= (LEVEL)) { CAGE_LOG(SeverityEnum::Info, "udp", stringizer() + MSG); } }

		struct MemView
		{
			MemView() : offset(0), size(0)
			{}

			MemView(const std::shared_ptr<MemoryBuffer> &buffer, uintPtr offset, uintPtr size) : buffer(buffer), offset(offset), size(size)
			{}

			Deserializer des() const
			{
				Deserializer d(*buffer);
				d.advance(offset);
				return d.placeholder(size);
			}

			std::shared_ptr<MemoryBuffer> buffer;
			uintPtr offset;
			uintPtr size;
		};

		struct SockGroup
		{
			SockGroup()
			{
				mut = newMutex();
			}

			struct Receiver
			{
				Receiver() : sockIndex(m), connId(0)
				{}

				Addr address;
				std::vector<MemView> packets;
				uint32 sockIndex;
				uint32 connId;
			};

			std::map<uint32, std::weak_ptr<Receiver>> receivers;
			std::weak_ptr<std::vector<std::shared_ptr<Receiver>>> accepting;
			std::vector<Sock> socks;
			Holder<Mutex> mut;

			void applyBufferSizes()
			{
				for (Sock &s : socks)
					s.setBufferSize(confBufferSize);
			}

			void readAll()
			{
				for (uint32 sockIndex = 0; sockIndex < socks.size(); sockIndex++)
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
							if (avail < 8)
								break;
							std::shared_ptr<MemoryBuffer> buff = std::make_shared<MemoryBuffer>();
							buff->resize(avail);
							uintPtr off = 0;
							while (avail)
							{
								uintPtr siz = s.recvFrom(buff->data() + off, avail, adr);
								CAGE_ASSERT(siz <= avail, siz, avail, off);
								MemView mv(buff, off, siz);
								avail -= siz;
								off += siz;
								if (siz < 8)
								{
									UDP_LOG(7, "received invalid packet (too small)");
									continue;
								}
								Deserializer des = mv.des();
								{ // read signature
									char c, a, g, e;
									des >> c >> a >> g >> e;
									if (c != 'c' || a != 'a' || g != 'g' || e != 'e')
									{
										UDP_LOG(7, "received invalid packet (wrong signature)");
										continue;
									}
								}
								uint32 connId;
								des >> connId;
								auto r = receivers[connId].lock();
								if (r)
								{
									r->packets.push_back(mv);
									if (r->sockIndex == m)
									{
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
			return (a < b && b - a < 32768)
				|| (a > b && a - b > 32768);
		}

		static_assert(comp(5, 10), "compare sequence numbers");
		static_assert(comp(50000, 60000), "compare sequence numbers");
		static_assert(comp(60000, 5), "compare sequence numbers");
		static_assert(!comp(10, 5), "compare sequence numbers");
		static_assert(!comp(60000, 50000), "compare sequence numbers");
		static_assert(!comp(5, 60000), "compare sequence numbers");
		static_assert(!comp(5, 5), "compare sequence numbers");
		static_assert(!comp(60000, 60000), "compare sequence numbers");

		std::set<uint16> decodeAck(uint16 seqn, uint32 bits)
		{
			std::set<uint16> result;
			for (uint16 i = 0; i < 32; i++)
			{
				uint32 m = 1 << i;
				if ((bits & m) == m)
				{
					uint16 s = seqn - i;
					result.insert(s);
				}
			}
			return result;
		}

		uint32 encodeAck(uint16 seqn, const std::set<uint16> &bits)
		{
			uint32 result = 0;
			for (uint16 i = 0; i < 32; i++)
			{
				uint16 s = seqn - i;
				if (bits.count(s))
				{
					uint32 m = 1 << i;
					result |= m;
				}
			}
			return result;
		}

#ifdef CAGE_DEBUG
		class AckTester
		{
		public:
			AckTester()
			{
				CAGE_ASSERT(decodeAck(1000, encodeAck(1000, { 999 })) == std::set<uint16>({ 999 }));
				CAGE_ASSERT(decodeAck(1000, encodeAck(1000, { 1000 })) == std::set<uint16>({ 1000 }));
				CAGE_ASSERT(decodeAck(1000, encodeAck(1000, { 1000, 999 })) == std::set<uint16>({ 1000, 999 }));
				CAGE_ASSERT(decodeAck(1000, encodeAck(1000, { 1000, 999, 990 })) == std::set<uint16>({ 1000, 999, 990 }));
				CAGE_ASSERT(decodeAck(5, encodeAck(5, { 1, 65533 })) == std::set<uint16>({ 1, 65533 }));
			}
		} ackTesterInstance;
#endif // CAGE_DEBUG

		struct PackAck
		{
			uint32 ackBits;
			uint16 ackSeqn;

			PackAck() : ackBits(0), ackSeqn(0)
			{}

			PackAck(uint16 seqn, uint32 bits) : ackBits(bits), ackSeqn(seqn)
			{}

			bool operator < (const PackAck &other) const
			{
				if (ackSeqn == other.ackSeqn)
					return ackBits < other.ackBits;
				return ackSeqn < other.ackSeqn;
			}
		};

		enum class CmdTypeEnum : uint8
		{
			invalid = 0,
			connectionInit = 1, // uint16 responseIndex, uint16 LongSize
			connectionFinish = 2,
			acknowledgement = 13, // uint16 packetSeqn, uint32 bits
			shortMessage = 20, // uint8 channel, uint16 msgSeqn, uint16 size, data
			longMessage = 21, // uint8 channel, uint16 msgSeqn, uint32 totalSize, uint16 index, data
			statsDiscovery = 42, // uint64 aReceivedBytes, uint64 aSentBytes, uint64 aTime, uint64 bReceivedBytes, uint64 bSentBytes, uint64 bTime, uint32 aReceivedPackets, uint32 aSentPackets, uint32 bReceivedPackets, uint32 bSentPackets, uint16 step
			mtuDiscovery = 43, // todo
		};

		const uint16 LongSize = 470; // designed to work well with default mtu (fits 3 long message commands in single packet)

		uint16 longCmdsCount(uint32 totalSize)
		{
			return totalSize / LongSize + ((totalSize % LongSize) == 0 ? 0 : 1);
		}

		class UdpConnectionImpl : public UdpConnection
		{
		public:
			UdpConnectionImpl(const string &address, uint16 port, uint64 timeout) : statsLastTimestamp(0), startTime(getApplicationTime()), connId(randomRange(1u, detail::numeric_limits<uint32>::max())), established(false)
			{
				UDP_LOG(1, "creating new connection to address: '" + address + "', port: " + port + ", timeout: " + timeout);
				sockGroup = std::make_shared<SockGroup>();
				AddrList lst(address, port, AF_UNSPEC, SOCK_DGRAM, IPPROTO_UDP, AI_PASSIVE);
				while (lst.valid())
				{
					Addr adr;
					int family, type, protocol;
					lst.getAll(adr, family, type, protocol);
					Sock s(family, type, protocol);
					s.setBlocking(false);
					s.connect(adr);
					if (s.isValid())
						sockGroup->socks.push_back(std::move(s));
					lst.next();
				}
				sockGroup->applyBufferSizes();
				if (sockGroup->socks.empty())
					CAGE_THROW_ERROR(Exception, "failed to connect (no sockets available)");
				UDP_LOG(2, "created " + sockGroup->socks.size() + " sockets");
				sockReceiver = std::make_shared<SockGroup::Receiver>();
				sockReceiver->connId = connId;
				sockGroup->receivers[connId] = sockReceiver;
				{ // initialize connection
					service();
					if (timeout)
					{
						while (true)
						{
							threadSleep(5000);
							service();
							if (established)
								break;
							if (getApplicationTime() > startTime + timeout)
								CAGE_THROW_ERROR(Disconnected, "failed to connect (timeout)");
						}
					}
				}
			}

			UdpConnectionImpl(std::shared_ptr<SockGroup> sg, std::shared_ptr<SockGroup::Receiver> rec) : statsLastTimestamp(0), sockGroup(sg), sockReceiver(rec), startTime(getApplicationTime()), connId(rec->connId), established(false)
			{
				UDP_LOG(1, "accepting new connection");
			}

			~UdpConnectionImpl()
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
						cmd.type = CmdTypeEnum::connectionFinish;
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
					float ch = confSimulatedPacketLoss;
					if (ch > 0 && randomChance() < ch)
					{
						UDP_LOG(4, "dropping packet due to simulated packet loss");
						return;
					}
				}

				// sending does not need to be under mutex
				if (sockReceiver->sockIndex == m)
				{
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
					Sock &s = sockGroup->socks[sockReceiver->sockIndex];
					if (s.isValid())
					{
						if (s.getConnected())
						{
							CAGE_ASSERT(s.getRemoteAddress() == sockReceiver->address);
							s.send(data, size);
						}
						else
							s.sendTo(data, size, sockReceiver->address);
					}
				}
			}

			struct Sending
			{
				struct ReliableMsg
				{
					std::shared_ptr<MemoryBuffer> data;
					std::vector<bool> parts; // acked parts
					uint16 step;
					uint16 msgSeqn;
					uint8 channel;

					ReliableMsg() : step(0), msgSeqn(0), channel(0)
					{}
				};

				struct MsgAck
				{
					std::weak_ptr<ReliableMsg> msg;
					uint16 index;

					MsgAck() : index(0)
					{}
				};

				union CommandUnion
				{
					uint16 initIndex;

					PackAck ack;

					struct Msg
					{
						uint16 msgSeqn;
						uint8 channel;
					} msg;

					struct Stats
					{
						struct sideStruct
						{
							uint64 time;
							uint64 receivedBytes;
							uint64 sentBytes;
							uint32 receivedPackets;
							uint32 sentPackets;
						} a, b;
						uint16 step;

						Stats()
						{
							detail::memset(this, 0, sizeof(*this));
						}
					} stats;

					CommandUnion()
					{
						detail::memset(this, 0, sizeof(*this));
					}

					~CommandUnion()
					{}
				};

				struct Command
				{
					CommandUnion data;
					MsgAck msgAck;
					MemView msgData;
					CmdTypeEnum type;
					sint8 priority;

					Command() : type(CmdTypeEnum::invalid), priority(0)
					{}
				};

				std::list<std::shared_ptr<ReliableMsg>> relMsgs;
				std::list<Command> cmds;
				std::map<uint16, std::vector<MsgAck>> ackMap; // mapping packet seqn to message parts
				std::array<uint16, 256> seqnPerChannel; // next message seqn to be used
				std::set<uint16> seqnToAck; // packets seqn to be acked
				uint16 packetSeqn; // next packet seqn to be used

				Sending() : packetSeqn(0)
				{
					for (uint16 &s : seqnPerChannel)
						s = 0;
				}
			} sending;

			void generateCommands(std::shared_ptr<Sending::ReliableMsg> &msg, sint8 priority)
			{
				Sending::Command cmd;
				cmd.msgData.buffer = msg->data;
				cmd.data.msg.msgSeqn = msg->msgSeqn;
				cmd.data.msg.channel = msg->channel;
				cmd.priority = priority;
				if (msg->channel >= 128)
					cmd.msgAck.msg = msg;

				bool completelyAcked = true;

				if (msg->data->size() > LongSize)
				{ // long message
					cmd.type = CmdTypeEnum::longMessage;
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
						cmd.type = CmdTypeEnum::shortMessage;
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
				std::set<uint16> tmp;
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
					std::set<uint16> ver;
					for (PackAck pa : acks)
					{
						std::set<uint16> c = decodeAck(pa.ackSeqn, pa.ackBits);
						ver.insert(c.begin(), c.end());
					}
					CAGE_ASSERT(ver == sending.seqnToAck);
				}
#endif // CAGE_ASSERT_ENABLED

				for (const PackAck &pa : acks)
				{
					Sending::Command cmd;
					cmd.data.ack = pa;
					cmd.type = CmdTypeEnum::acknowledgement;
					cmd.priority = priority;
					sending.cmds.push_back(templates::move(cmd));
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
					if (priority >= 0)
						generateCommands(msg, priority);
					if (msg && msg->step++ >= 300)
						CAGE_THROW_ERROR(Disconnected, "too many failed attempts at sending a reliable message");
				}
			}

			void serializeCommand(const Sending::Command &cmd, Serializer &ser)
			{
				ser << cmd.type;
				switch (cmd.type)
				{
				case CmdTypeEnum::connectionInit:
				{
					ser << cmd.data.initIndex << LongSize;
				} break;
				case CmdTypeEnum::connectionFinish:
				{
					// nothing
				} break;
				case CmdTypeEnum::acknowledgement:
				{
					ser << cmd.data.ack.ackSeqn << cmd.data.ack.ackBits;
				} break;
				case CmdTypeEnum::shortMessage:
				{
					ser << cmd.data.msg.channel << cmd.data.msg.msgSeqn;
					uint16 size = numeric_cast<uint16>(cmd.msgData.size);
					ser << size;
					ser.write(cmd.msgData.buffer->data(), size);
				} break;
				case CmdTypeEnum::longMessage:
				{
					ser << cmd.data.msg.channel << cmd.data.msg.msgSeqn;
					uint32 totalSize = numeric_cast<uint32>(cmd.msgData.buffer->size());
					uint16 index = numeric_cast<uint16>(cmd.msgData.offset / LongSize);
					ser << totalSize << index;
					ser.write(cmd.msgData.buffer->data() + cmd.msgData.offset, cmd.msgData.size);
				} break;
				case CmdTypeEnum::statsDiscovery:
				{
					const auto &s = cmd.data.stats;
					ser << s.a.receivedBytes << s.a.sentBytes << s.a.time << s.b.receivedBytes << s.b.sentBytes << s.b.time << s.a.receivedPackets << s.a.sentPackets << s.b.receivedPackets << s.b.sentPackets << s.step;
				} break;
				default:
					CAGE_THROW_CRITICAL(Exception, "invalid udp command type enum");
				}
			}

			void composePackets()
			{
				const uint32 mtu = confMtu;
				MemoryBuffer buff;
				buff.reserve(mtu);
				Serializer ser(buff);
				uint16 currentPacketSeqn = 0;
				bool empty = true;
				for (const Sending::Command &cmd : sending.cmds)
				{
					uint32 cmdSize = numeric_cast<uint32>(cmd.msgData.size) + 10;

					// send current packet
					if (!empty && buff.size() + cmdSize > mtu)
					{
						dispatchPacket(buff.data(), buff.size());
						buff.resize(0);
						ser = Serializer(buff);
						empty = true;
					}

					// generate packet header
					if (buff.size() == 0)
					{
						static const char c = 'c', a = 'a', g = 'g', e = 'e';
						ser << c << a << g << e << connId;
						currentPacketSeqn = sending.packetSeqn++;
						ser << currentPacketSeqn;
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
					cmd.type = CmdTypeEnum::connectionInit;
					cmd.priority = 10;
					sending.cmds.push_back(cmd);
				}

				if (getApplicationTime() > statsLastTimestamp + 1000000)
				{
					Sending::CommandUnion::Stats p;
					handleStats(p);
				}

				generateAckCommands(0);
				resendReliableMessages();

				sending.cmds.sort([](const Sending::Command &a, const Sending::Command &b) {
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
					auto it = sending.relMsgs.begin();
					auto et = sending.relMsgs.end();
					while (it != et)
					{
						if (!*it)
							it = sending.relMsgs.erase(it);
						else
							it++;
					}
				}

				{ // clear ackMap
					auto it = sending.ackMap.begin();
					auto et = sending.ackMap.end();
					while (it != et)
					{
						it->second.erase(std::remove_if(it->second.begin(), it->second.end(), [](Sending::MsgAck &p){
							return !p.msg.lock();
						}), it->second.end());
						if (it->second.empty())
							it = sending.ackMap.erase(it);
						else
							it++;
					}
				}
			}

			// RECEIVING

			struct Receiving
			{
				struct Msg
				{
					MemoryBuffer data;
					std::vector<bool> parts; // acked parts
					uint16 msgSeqn;
					uint8 channel;

					Msg() : msgSeqn(0), channel(0)
					{}
				};

				std::array<std::map<uint16, Msg>, 256> staging;
				std::array<uint16, 256> seqnPerChannel; // minimum expected message seqn
				std::list<Msg> messages;
				std::set<PackAck> ackPacks;
				uint16 packetSeqn; // minimum expected packet seqn

				Receiving() : packetSeqn(0)
				{
					for (uint16 &s : seqnPerChannel)
						s = 0;
				}
			} receiving;

			void processAcks()
			{
				std::set<uint16> acks;
				for (const auto &a : receiving.ackPacks)
				{
					std::set<uint16> s = decodeAck(a.ackSeqn, a.ackBits);
#if __cplusplus >= 201703L
					acks.merge(s);
#else
					acks.insert(s.begin(), s.end());
#endif
				}
				receiving.ackPacks.clear();
				for (uint16 ack : acks)
				{
					if (sending.ackMap.count(ack) == 0)
						continue;
					for (auto &ma : sending.ackMap[ack])
					{
						auto m = ma.msg.lock();
						if (m)
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
					std::vector<Receiving::Msg*> msgs;
					msgs.reserve(stage.size());
					for (auto &it : stage)
						msgs.push_back(&it.second);
					std::sort(msgs.begin(), msgs.end(), [](const Receiving::Msg *a, Receiving::Msg *b) {
						return comp(a->msgSeqn, b->msgSeqn);
					});
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
						receiving.messages.push_back(templates::move(*msg));
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
						receiving.messages.push_back(templates::move(msg));
					}
				}

				// erase empty
				for (uint32 channel = 0; channel < 255; channel++)
				{
					auto &stage = receiving.staging[channel];
					auto it = stage.begin();
					auto et = stage.end();
					while (it != et)
					{
						if (it->second.data.size() == 0)
							it = stage.erase(it);
						else
							it++;
					}
				}
			}

			void handleStats(Sending::CommandUnion::Stats &p)
			{
				UdpStatistics &s = stats;
				statsLastTimestamp = getApplicationTime();
				if (p.step > 0)
				{
					s.roundTripDuration = statsLastTimestamp - p.b.time;
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
				p.a.time = statsLastTimestamp;
				p.step++;
				Sending::Command cmd;
				cmd.data.stats = p;
				cmd.type = CmdTypeEnum::statsDiscovery;
				cmd.priority = 1;
				sending.cmds.push_back(templates::move(cmd));
			}

			void handleReceivedCommand(Deserializer &d)
			{
				CmdTypeEnum type;
				d >> type;
				switch (type)
				{
				case CmdTypeEnum::connectionInit:
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
							string a;
							uint16 p;
							sockReceiver->address.translate(a, p, true);
							CAGE_LOG(SeverityEnum::Info, "udp", stringizer() + "connection established, remote address: '" + a + "', remote port: " + p);
						}
						established = true;
					}
					if (index == 0)
					{
						Sending::Command cmd;
						cmd.data.initIndex = index + 1;
						cmd.type = CmdTypeEnum::connectionInit;
						cmd.priority = 10;
						sending.cmds.push_back(cmd);
					}
				} break;
				case CmdTypeEnum::connectionFinish:
				{
					CAGE_THROW_ERROR(Disconnected, "connection closed by other end");
				} break;
				case CmdTypeEnum::acknowledgement:
				{
					PackAck ackPack;
					d >> ackPack.ackSeqn >> ackPack.ackBits;
					receiving.ackPacks.insert(ackPack);
				} break;
				case CmdTypeEnum::shortMessage:
				{
					Receiving::Msg msg;
					uint16 size = 0;
					d >> msg.channel >> msg.msgSeqn >> size;
					if (comp(msg.msgSeqn, receiving.seqnPerChannel[msg.channel]))
					{ // obsolete
						d.advance(size);
					}
					else
					{
						msg.data.resize(size);
						d.read(msg.data.data(), size);
						receiving.staging[msg.channel][msg.msgSeqn] = templates::move(msg);
					}
				} break;
				case CmdTypeEnum::longMessage:
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
						d.advance(size);
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
						d.read(msg.data.data() + index * LongSize, size);
					}
				} break;
				case CmdTypeEnum::statsDiscovery:
				{
					Sending::CommandUnion::Stats s;
					d >> s.a.receivedBytes >> s.a.sentBytes >> s.a.time >> s.b.receivedBytes >> s.b.sentBytes >> s.b.time >> s.a.receivedPackets >> s.a.sentPackets >> s.b.receivedPackets >> s.b.sentPackets >> s.step;
					handleStats(s);
				} break;
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
					uint32 sign, id;
					d >> sign >> id;
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
					ScopeLock<Mutex> lock(sockGroup->mut);
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

			// COMMON

			UdpStatistics stats;
			std::shared_ptr<SockGroup> sockGroup;
			std::shared_ptr<SockGroup::Receiver> sockReceiver;
			uint64 statsLastTimestamp;
			const uint64 startTime;
			const uint32 connId;
			bool established;

			// API

			uintPtr available()
			{
				if (receiving.messages.empty())
					return 0;
				return receiving.messages.front().data.size();
			}

			MemoryBuffer read(uint32 &channel, bool &reliable)
			{
				if (receiving.messages.empty())
					CAGE_THROW_ERROR(Exception, "no data available");
				auto tmp = templates::move(receiving.messages.front());
				receiving.messages.pop_front();
				channel = tmp.channel & 127; // strip reliability bit
				reliable = tmp.channel > 127;
				return templates::move(tmp.data);
			}

			void write(const MemoryBuffer &buffer, uint32 channel, bool reliable)
			{
				CAGE_ASSERT(channel < 128, channel, reliable);
				CAGE_ASSERT(buffer.size() <= 16 * 1024 * 1024, buffer.size(), channel, reliable);
				if (buffer.size() == 0)
					return; // ignore empty messages

				auto msg = std::make_shared<Sending::ReliableMsg>();
				msg->data = std::make_shared<MemoryBuffer>(buffer.copy());
				msg->channel = numeric_cast<uint8>(channel + reliable * 128);
				msg->msgSeqn = sending.seqnPerChannel[msg->channel]++;
				if (reliable)
				{
					msg->parts.resize(longCmdsCount(numeric_cast<uint32>(msg->data->size())));
					sending.relMsgs.push_back(templates::move(msg));
				}
				else
					generateCommands(msg, 0);
			}

			void service()
			{
				detail::OverrideBreakpoint brk;
				serviceReceiving();
				serviceSending();
			}
		};

		class UdpServerImpl : public UdpServer
		{
		public:
			UdpServerImpl(uint16 port)
			{
				UDP_LOG(1, "creating new server on port " + port);
				sockGroup = std::make_shared<SockGroup>();
				AddrList lst(nullptr, port, AF_UNSPEC, SOCK_DGRAM, IPPROTO_UDP, AI_PASSIVE);
				while (lst.valid())
				{
					Addr adr;
					int family, type, protocol;
					lst.getAll(adr, family, type, protocol);
					Sock s(family, type, protocol);
					s.setBlocking(false);
					s.setReuseaddr(true);
					s.bind(adr);
					if (s.isValid())
						sockGroup->socks.push_back(std::move(s));
					lst.next();
				}
				sockGroup->applyBufferSizes();
				if (sockGroup->socks.empty())
					CAGE_THROW_ERROR(Exception, "failed to bind (no sockets available)");
				accepting = std::make_shared<std::vector<std::shared_ptr<SockGroup::Receiver>>>();
				sockGroup->accepting = accepting;
				UDP_LOG(2, "listening on " + sockGroup->socks.size() + " sockets");
			}

			~UdpServerImpl()
			{
				UDP_LOG(2, "destroying server");
			}

			Holder<UdpConnection> accept()
			{
				detail::OverrideBreakpoint brk;
				std::shared_ptr<SockGroup::Receiver> acc;
				{
					ScopeLock<Mutex> lock(sockGroup->mut);
					sockGroup->readAll();
					if (accepting->empty())
						return {};
					acc = accepting->back();
					accepting->pop_back();
				}
				try
				{
					auto c = detail::systemArena().createHolder<UdpConnectionImpl>(sockGroup, acc);
					c->serviceReceiving();
					if (!c->established)
					{
						UDP_LOG(2, "received packets failed to initialize new connection");
						return {};
					}
					return c.cast<UdpConnection>();
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

	uintPtr UdpConnection::available()
	{
		UdpConnectionImpl *impl = (UdpConnectionImpl*)this;
		return impl->available();
	}

	uintPtr UdpConnection::read(void *buffer, uintPtr size, uint32 &channel, bool &reliable)
	{
		MemoryBuffer b = read(channel, reliable);
		if (b.size() > size)
			CAGE_THROW_ERROR(Exception, "buffer is too small");
		detail::memcpy(buffer, b.data(), b.size());
		return b.size();
	}

	uintPtr UdpConnection::read(void *buffer, uintPtr size)
	{
		uint32 channel;
		bool reliable;
		return read(buffer, size, channel, reliable);
	}

	MemoryBuffer UdpConnection::read(uint32 &channel, bool &reliable)
	{
		UdpConnectionImpl *impl = (UdpConnectionImpl*)this;
		return impl->read(channel, reliable);
	}

	MemoryBuffer UdpConnection::read()
	{
		uint32 channel;
		bool reliable;
		return read(channel, reliable);
	}

	void UdpConnection::write(const void *buffer, uintPtr size, uint32 channel, bool reliable)
	{
		MemoryBuffer b(size);
		detail::memcpy(b.data(), buffer, size);
		write(b, channel, reliable);
	}

	void UdpConnection::write(const MemoryBuffer &buffer, uint32 channel, bool reliable)
	{
		UdpConnectionImpl *impl = (UdpConnectionImpl*)this;
		impl->write(buffer, channel, reliable);
	}

	void UdpConnection::update()
	{
		UdpConnectionImpl *impl = (UdpConnectionImpl*)this;
		impl->service();
	}

	const UdpStatistics &UdpConnection::statistics() const
	{
		const UdpConnectionImpl *impl = (const UdpConnectionImpl*)this;
		return impl->stats;
	}

	Holder<UdpConnection> UdpServer::accept()
	{
		UdpServerImpl *impl = (UdpServerImpl*)this;
		return impl->accept();
	}

	Holder<UdpConnection> newUdpConnection(const string &address, uint16 port, uint64 timeout)
	{
		return detail::systemArena().createImpl<UdpConnection, UdpConnectionImpl>(address, port, timeout);
	}

	Holder<UdpServer> newUdpServer(uint16 port)
	{
		return detail::systemArena().createImpl<UdpServer, UdpServerImpl>(port);
	}

	UdpStatistics::UdpStatistics()
	{
		detail::memset(this, 0, sizeof(*this));
	}
}
