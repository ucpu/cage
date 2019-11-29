#include <array>
#include <vector>
#include <set>
#include <map>
#include <list>
#include <memory>
#include <algorithm>

#include "net.h"
#include <cage-core/math.h> // random
#include <cage-core/config.h>
#include <cage-core/concurrent.h>
#include <cage-core/serialization.h>

namespace cage
{
	// nomenclature:
	// message - udpConnection methods read and write are working with messages
	// packet - system functions work with packets
	// command - messages are fragmented into commands which are grouped into packets

	namespace
	{
		using namespace privat;

		configUint32 logLevel("cage.udp.logLevel", 0);
		configFloat confSimulatedPacketLoss("cage.udp.simulatedPacketLoss", 0);
		configUint32 confMtu("cage.udp.maxTransferUnit", 1450);
		configUint32 confBufferSize("cage.udp.systemBufferSize", 1024 * 1024);

#define UDP_LOG(LEVEL, MSG) { if (logLevel >= (LEVEL)) { CAGE_LOG(severityEnum::Info, "udp", stringizer() + MSG); } }

		struct memView
		{
			memView() : offset(0), size(0)
			{}

			memView(const std::shared_ptr<memoryBuffer> &buffer, uintPtr offset, uintPtr size) : buffer(buffer), offset(offset), size(size)
			{}

			deserializer des() const
			{
				deserializer d(*buffer);
				d.advance(offset);
				return d.placeholder(size);
			}

			std::shared_ptr<memoryBuffer> buffer;
			uintPtr offset;
			uintPtr size;
		};

		struct sockGroupStruct
		{
			sockGroupStruct()
			{
				mut = newSyncMutex();
			}

			struct receiverStruct
			{
				receiverStruct() : sockIndex(m), connId(0)
				{}

				addr address;
				std::vector<memView> packets;
				uint32 sockIndex;
				uint32 connId;
			};

			std::map<uint32, std::weak_ptr<receiverStruct>> receivers;
			std::weak_ptr<std::vector<std::shared_ptr<receiverStruct>>> accepting;
			std::vector<sock> socks;
			holder<syncMutex> mut;

			void applyBufferSizes()
			{
				for (sock &s : socks)
					s.setBufferSize(confBufferSize);
			}

			void readAll()
			{
				for (uint32 sockIndex = 0; sockIndex < socks.size(); sockIndex++)
				{
					sock &s = socks[sockIndex];
					if (!s.isValid())
						continue;
					try
					{
						addr adr;
						while (true)
						{
							auto avail = s.available();
							if (avail < 8)
								break;
							std::shared_ptr<memoryBuffer> buff = std::make_shared<memoryBuffer>();
							buff->resize(avail);
							uintPtr off = 0;
							while (avail)
							{
								uintPtr siz = s.recvFrom(buff->data() + off, avail, adr);
								CAGE_ASSERT(siz <= avail, siz, avail, off);
								memView mv(buff, off, siz);
								avail -= siz;
								off += siz;
								if (siz < 8)
								{
									UDP_LOG(7, "received invalid packet (too small)");
									continue;
								}
								deserializer des = mv.des();
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
									auto s = std::make_shared<receiverStruct>();
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
		class ackTesterClass
		{
		public:
			ackTesterClass()
			{
				CAGE_ASSERT(decodeAck(1000, encodeAck(1000, { 999 })) == std::set<uint16>({ 999 }));
				CAGE_ASSERT(decodeAck(1000, encodeAck(1000, { 1000 })) == std::set<uint16>({ 1000 }));
				CAGE_ASSERT(decodeAck(1000, encodeAck(1000, { 1000, 999 })) == std::set<uint16>({ 1000, 999 }));
				CAGE_ASSERT(decodeAck(1000, encodeAck(1000, { 1000, 999, 990 })) == std::set<uint16>({ 1000, 999, 990 }));
				CAGE_ASSERT(decodeAck(5, encodeAck(5, { 1, 65533 })) == std::set<uint16>({ 1, 65533 }));
			}
		} ackTesterInstance;
#endif // CAGE_DEBUG

		struct packAckStruct
		{
			uint32 ackBits;
			uint16 ackSeqn;

			packAckStruct() : ackBits(0), ackSeqn(0)
			{}

			packAckStruct(uint16 seqn, uint32 bits) : ackBits(bits), ackSeqn(seqn)
			{}

			bool operator < (const packAckStruct &other) const
			{
				if (ackSeqn == other.ackSeqn)
					return ackBits < other.ackBits;
				return ackSeqn < other.ackSeqn;
			}
		};

		enum class cmdTypeEnum : uint8
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

		class udpConnectionImpl : public udpConnection
		{
		public:
			udpConnectionImpl(const string &address, uint16 port, uint64 timeout) : statsLastTimestamp(0), startTime(getApplicationTime()), connId(randomRange(1u, detail::numeric_limits<uint32>::max())), established(false)
			{
				UDP_LOG(1, "creating new connection to address: '" + address + "', port: " + port + ", timeout: " + timeout);
				sockGroup = std::make_shared<sockGroupStruct>();
				addrList lst(address, port, AF_UNSPEC, SOCK_DGRAM, IPPROTO_UDP, AI_PASSIVE);
				while (lst.valid())
				{
					addr adr;
					int family, type, protocol;
					lst.getAll(adr, family, type, protocol);
					sock s(family, type, protocol);
					s.setBlocking(false);
					s.connect(adr);
					if (s.isValid())
						sockGroup->socks.push_back(std::move(s));
					lst.next();
				}
				sockGroup->applyBufferSizes();
				if (sockGroup->socks.empty())
					CAGE_THROW_ERROR(exception, "failed to connect (no sockets available)");
				UDP_LOG(2, "created " + sockGroup->socks.size() + " sockets");
				sockReceiver = std::make_shared<sockGroupStruct::receiverStruct>();
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
								CAGE_THROW_ERROR(disconnected, "failed to connect (timeout)");
						}
					}
				}
			}

			udpConnectionImpl(std::shared_ptr<sockGroupStruct> sg, std::shared_ptr<sockGroupStruct::receiverStruct> rec) : statsLastTimestamp(0), sockGroup(sg), sockReceiver(rec), startTime(getApplicationTime()), connId(rec->connId), established(false)
			{
				UDP_LOG(1, "accepting new connection");
			}

			~udpConnectionImpl()
			{
				UDP_LOG(2, "destroying connection");

				// send connection closed packet
				if (established)
				{
					detail::overrideBreakpoint ob;
					try
					{
						sending.cmds.clear();
						sendingStruct::commandStruct cmd;
						cmd.type = cmdTypeEnum::connectionFinish;
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
					for (sock &s : sockGroup->socks)
					{
						if (!s.isValid())
							continue;
						CAGE_ASSERT(s.getConnected());
						s.send(data, size);
					}
				}
				else
				{
					sock &s = sockGroup->socks[sockReceiver->sockIndex];
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

			struct sendingStruct
			{
				struct reliableMsgStruct
				{
					std::shared_ptr<memoryBuffer> data;
					std::vector<bool> parts; // acked parts
					uint16 step;
					uint16 msgSeqn;
					uint8 channel;

					reliableMsgStruct() : step(0), msgSeqn(0), channel(0)
					{}
				};

				struct msgAckStruct
				{
					std::weak_ptr<reliableMsgStruct> msg;
					uint16 index;

					msgAckStruct() : index(0)
					{}
				};

				union commandUnion
				{
					uint16 initIndex;

					packAckStruct ack;

					struct msgStruct
					{
						uint16 msgSeqn;
						uint8 channel;
					} msg;

					struct statsStruct
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

						statsStruct()
						{
							detail::memset(this, 0, sizeof(*this));
						}
					} stats;

					commandUnion()
					{
						detail::memset(this, 0, sizeof(*this));
					}

					~commandUnion()
					{}
				};

				struct commandStruct
				{
					commandUnion data;
					msgAckStruct msgAck;
					memView msgData;
					cmdTypeEnum type;
					sint8 priority;

					commandStruct() : type(cmdTypeEnum::invalid), priority(0)
					{}
				};

				std::list<std::shared_ptr<reliableMsgStruct>> relMsgs;
				std::list<commandStruct> cmds;
				std::map<uint16, std::vector<msgAckStruct>> ackMap; // mapping packet seqn to message parts
				std::array<uint16, 256> seqnPerChannel; // next message seqn to be used
				std::set<uint16> seqnToAck; // packets seqn to be acked
				uint16 packetSeqn; // next packet seqn to be used

				sendingStruct() : packetSeqn(0)
				{
					for (uint16 &s : seqnPerChannel)
						s = 0;
				}
			} sending;

			void generateCommands(std::shared_ptr<sendingStruct::reliableMsgStruct> &msg, sint8 priority)
			{
				sendingStruct::commandStruct cmd;
				cmd.msgData.buffer = msg->data;
				cmd.data.msg.msgSeqn = msg->msgSeqn;
				cmd.data.msg.channel = msg->channel;
				cmd.priority = priority;
				if (msg->channel >= 128)
					cmd.msgAck.msg = msg;

				bool completelyAcked = true;

				if (msg->data->size() > LongSize)
				{ // long message
					cmd.type = cmdTypeEnum::longMessage;
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
						cmd.type = cmdTypeEnum::shortMessage;
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

				std::vector<packAckStruct> acks;
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
						packAckStruct p(front, encodeAck(front, tmp));
						CAGE_ASSERT(decodeAck(p.ackSeqn, p.ackBits) == tmp);
						acks.push_back(p);
						tmp.clear();
						front = n;
						tmp.insert(front);
					}
				}
				{
					packAckStruct p(front, encodeAck(front, tmp));
					CAGE_ASSERT(decodeAck(p.ackSeqn, p.ackBits) == tmp);
					acks.push_back(p);
				}

#ifdef CAGE_ASSERT_ENABLED
				{ // verification
					std::set<uint16> ver;
					for (packAckStruct pa : acks)
					{
						std::set<uint16> c = decodeAck(pa.ackSeqn, pa.ackBits);
						ver.insert(c.begin(), c.end());
					}
					CAGE_ASSERT(ver == sending.seqnToAck);
				}
#endif // CAGE_ASSERT_ENABLED

				for (const packAckStruct &pa : acks)
				{
					sendingStruct::commandStruct cmd;
					cmd.data.ack = pa;
					cmd.type = cmdTypeEnum::acknowledgement;
					cmd.priority = priority;
					sending.cmds.push_back(templates::move(cmd));
				}
			}

			void resendReliableMessages()
			{
				for (std::shared_ptr<sendingStruct::reliableMsgStruct> &msg : sending.relMsgs)
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
						CAGE_THROW_ERROR(disconnected, "too many failed attempts at sending a reliable message");
				}
			}

			void serializeCommand(const sendingStruct::commandStruct &cmd, serializer &ser)
			{
				ser << cmd.type;
				switch (cmd.type)
				{
				case cmdTypeEnum::connectionInit:
				{
					ser << cmd.data.initIndex << LongSize;
				} break;
				case cmdTypeEnum::connectionFinish:
				{
					// nothing
				} break;
				case cmdTypeEnum::acknowledgement:
				{
					ser << cmd.data.ack.ackSeqn << cmd.data.ack.ackBits;
				} break;
				case cmdTypeEnum::shortMessage:
				{
					ser << cmd.data.msg.channel << cmd.data.msg.msgSeqn;
					uint16 size = numeric_cast<uint16>(cmd.msgData.size);
					ser << size;
					ser.write(cmd.msgData.buffer->data(), size);
				} break;
				case cmdTypeEnum::longMessage:
				{
					ser << cmd.data.msg.channel << cmd.data.msg.msgSeqn;
					uint32 totalSize = numeric_cast<uint32>(cmd.msgData.buffer->size());
					uint16 index = numeric_cast<uint16>(cmd.msgData.offset / LongSize);
					ser << totalSize << index;
					ser.write(cmd.msgData.buffer->data() + cmd.msgData.offset, cmd.msgData.size);
				} break;
				case cmdTypeEnum::statsDiscovery:
				{
					const auto &s = cmd.data.stats;
					ser << s.a.receivedBytes << s.a.sentBytes << s.a.time << s.b.receivedBytes << s.b.sentBytes << s.b.time << s.a.receivedPackets << s.a.sentPackets << s.b.receivedPackets << s.b.sentPackets << s.step;
				} break;
				default:
					CAGE_THROW_CRITICAL(exception, "invalid udp command type enum");
				}
			}

			void composePackets()
			{
				const uint32 mtu = confMtu;
				memoryBuffer buff;
				buff.reserve(mtu);
				serializer ser(buff);
				uint16 currentPacketSeqn = 0;
				bool empty = true;
				for (const sendingStruct::commandStruct &cmd : sending.cmds)
				{
					uint32 cmdSize = numeric_cast<uint32>(cmd.msgData.size) + 10;

					// send current packet
					if (!empty && buff.size() + cmdSize > mtu)
					{
						dispatchPacket(buff.data(), buff.size());
						buff.resize(0);
						ser = serializer(buff);
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
					sendingStruct::commandStruct cmd;
					cmd.type = cmdTypeEnum::connectionInit;
					cmd.priority = 10;
					sending.cmds.push_back(cmd);
				}

				if (getApplicationTime() > statsLastTimestamp + 1000000)
				{
					sendingStruct::commandUnion::statsStruct p;
					handleStats(p);
				}

				generateAckCommands(0);
				resendReliableMessages();

				sending.cmds.sort([](const sendingStruct::commandStruct &a, const sendingStruct::commandStruct &b) {
					return a.priority > b.priority; // higher priority first
				});

				try
				{
					composePackets();
				}
				catch (const cage::exception &)
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
						it->second.erase(std::remove_if(it->second.begin(), it->second.end(), [](sendingStruct::msgAckStruct &p){
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

			struct receivingStruct
			{
				struct msgStruct
				{
					memoryBuffer data;
					std::vector<bool> parts; // acked parts
					uint16 msgSeqn;
					uint8 channel;

					msgStruct() : msgSeqn(0), channel(0)
					{}
				};

				std::array<std::map<uint16, msgStruct>, 256> staging;
				std::array<uint16, 256> seqnPerChannel; // minimum expected message seqn
				std::list<msgStruct> messages;
				std::set<packAckStruct> ackPacks;
				uint16 packetSeqn; // minimum expected packet seqn

				receivingStruct() : packetSeqn(0)
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
					std::vector<receivingStruct::msgStruct*> msgs;
					msgs.reserve(stage.size());
					for (auto &it : stage)
						msgs.push_back(&it.second);
					std::sort(msgs.begin(), msgs.end(), [](const receivingStruct::msgStruct *a, receivingStruct::msgStruct *b) {
						return comp(a->msgSeqn, b->msgSeqn);
					});
					for (receivingStruct::msgStruct *msg : msgs)
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
						receivingStruct::msgStruct &msg = stage[seqnpc];
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

			void handleStats(sendingStruct::commandUnion::statsStruct &p)
			{
				udpConnectionStatistics &s = stats;
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
				sendingStruct::commandStruct cmd;
				cmd.data.stats = p;
				cmd.type = cmdTypeEnum::statsDiscovery;
				cmd.priority = 1;
				sending.cmds.push_back(templates::move(cmd));
			}

			void handleReceivedCommand(deserializer &d)
			{
				cmdTypeEnum type;
				d >> type;
				switch (type)
				{
				case cmdTypeEnum::connectionInit:
				{
					uint16 index = 0;
					uint16 longSize = 0;
					d >> index >> longSize;
					UDP_LOG(3, "received connection init command with index " + index);
					if (longSize != LongSize)
					{
						UDP_LOG(3, "the connection has incompatible LongSize: " + longSize + ", my LongSize: " + LongSize);
						CAGE_THROW_ERROR(disconnected, "incompatible connection (LongSize)");
					}
					if (!established)
					{
						if (logLevel >= 2)
						{
							string a;
							uint16 p;
							sockReceiver->address.translate(a, p, true);
							CAGE_LOG(severityEnum::Info, "udp", stringizer() + "connection established, remote address: '" + a + "', remote port: " + p);
						}
						established = true;
					}
					if (index == 0)
					{
						sendingStruct::commandStruct cmd;
						cmd.data.initIndex = index + 1;
						cmd.type = cmdTypeEnum::connectionInit;
						cmd.priority = 10;
						sending.cmds.push_back(cmd);
					}
				} break;
				case cmdTypeEnum::connectionFinish:
				{
					CAGE_THROW_ERROR(disconnected, "connection closed by other end");
				} break;
				case cmdTypeEnum::acknowledgement:
				{
					packAckStruct ackPack;
					d >> ackPack.ackSeqn >> ackPack.ackBits;
					receiving.ackPacks.insert(ackPack);
				} break;
				case cmdTypeEnum::shortMessage:
				{
					receivingStruct::msgStruct msg;
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
				case cmdTypeEnum::longMessage:
				{
					uint8 channel = 0;
					uint16 msgSeqn = 0;
					uint32 totalSize = 0;
					uint16 index = 0;
					d >> channel >> msgSeqn >> totalSize >> index;
					uint16 totalCount = longCmdsCount(totalSize);
					if (index >= totalCount)
						CAGE_THROW_ERROR(exception, "invalid long message index");
					uint16 size = index + 1 == totalCount ? totalSize - (totalCount - 1) * LongSize : LongSize;
					if (comp(msgSeqn, receiving.seqnPerChannel[channel]))
					{ // obsolete
						d.advance(size);
					}
					else
					{
						receivingStruct::msgStruct &msg = receiving.staging[channel][msgSeqn];
						if (msg.data.size() == 0)
						{
							msg.data.resize(totalSize);
							msg.channel = channel;
							msg.msgSeqn = msgSeqn;
							msg.parts.resize(totalCount);
						}
						else if (msg.data.size() != totalSize)
							CAGE_THROW_ERROR(exception, "inconsistent message total size");
						msg.parts[index] = true;
						d.read(msg.data.data() + index * LongSize, size);
					}
				} break;
				case cmdTypeEnum::statsDiscovery:
				{
					sendingStruct::commandUnion::statsStruct s;
					d >> s.a.receivedBytes >> s.a.sentBytes >> s.a.time >> s.b.receivedBytes >> s.b.sentBytes >> s.b.time >> s.a.receivedPackets >> s.a.sentPackets >> s.b.receivedPackets >> s.b.sentPackets >> s.step;
					handleStats(s);
				} break;
				default:
					CAGE_THROW_ERROR(exception, "invalid message type enum");
				}
			}

			void handleReceivedPacket(const memView &b)
			{
				deserializer d = b.des();
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
				std::vector<memView> packets;
				{
					scopeLock<syncMutex> lock(sockGroup->mut);
					sockGroup->readAll();
					sockReceiver->packets.swap(packets);
				}
				try
				{
					for (memView &b : packets)
						handleReceivedPacket(b);
				}
				catch (const disconnected &)
				{
					throw;
				}
				catch (cage::exception &)
				{
					// do nothing
				}
				processAcks();
				processReceived();
			}

			// COMMON

			udpConnectionStatistics stats;
			std::shared_ptr<sockGroupStruct> sockGroup;
			std::shared_ptr<sockGroupStruct::receiverStruct> sockReceiver;
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

			memoryBuffer read(uint32 &channel, bool &reliable)
			{
				if (receiving.messages.empty())
					CAGE_THROW_ERROR(exception, "no data available");
				auto tmp = templates::move(receiving.messages.front());
				receiving.messages.pop_front();
				channel = tmp.channel & 127; // strip reliability bit
				reliable = tmp.channel > 127;
				return templates::move(tmp.data);
			}

			void write(const memoryBuffer &buffer, uint32 channel, bool reliable)
			{
				CAGE_ASSERT(channel < 128, channel, reliable);
				CAGE_ASSERT(buffer.size() <= 16 * 1024 * 1024, buffer.size(), channel, reliable);
				if (buffer.size() == 0)
					return; // ignore empty messages

				auto msg = std::make_shared<sendingStruct::reliableMsgStruct>();
				msg->data = std::make_shared<memoryBuffer>(buffer.copy());
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
				detail::overrideBreakpoint brk;
				serviceReceiving();
				serviceSending();
			}
		};

		class udpServerImpl : public udpServer
		{
		public:
			udpServerImpl(uint16 port)
			{
				UDP_LOG(1, "creating new server on port " + port);
				sockGroup = std::make_shared<sockGroupStruct>();
				addrList lst(nullptr, port, AF_UNSPEC, SOCK_DGRAM, IPPROTO_UDP, AI_PASSIVE);
				while (lst.valid())
				{
					addr adr;
					int family, type, protocol;
					lst.getAll(adr, family, type, protocol);
					sock s(family, type, protocol);
					s.setBlocking(false);
					s.setReuseaddr(true);
					s.bind(adr);
					if (s.isValid())
						sockGroup->socks.push_back(std::move(s));
					lst.next();
				}
				sockGroup->applyBufferSizes();
				if (sockGroup->socks.empty())
					CAGE_THROW_ERROR(exception, "failed to bind (no sockets available)");
				accepting = std::make_shared<std::vector<std::shared_ptr<sockGroupStruct::receiverStruct>>>();
				sockGroup->accepting = accepting;
				UDP_LOG(2, "listening on " + sockGroup->socks.size() + " sockets");
			}

			~udpServerImpl()
			{
				UDP_LOG(2, "destroying server");
			}

			holder<udpConnection> accept()
			{
				detail::overrideBreakpoint brk;
				std::shared_ptr<sockGroupStruct::receiverStruct> acc;
				{
					scopeLock<syncMutex> lock(sockGroup->mut);
					sockGroup->readAll();
					if (accepting->empty())
						return {};
					acc = accepting->back();
					accepting->pop_back();
				}
				try
				{
					auto c = detail::systemArena().createHolder<udpConnectionImpl>(sockGroup, acc);
					c->serviceReceiving();
					if (!c->established)
					{
						UDP_LOG(2, "received packets failed to initialize new connection");
						return {};
					}
					return c.cast<udpConnection>();
				}
				catch (...)
				{
					return {};
				}
			}

			std::shared_ptr<sockGroupStruct> sockGroup;
			std::shared_ptr<std::vector<std::shared_ptr<sockGroupStruct::receiverStruct>>> accepting;
		};
	}

	uintPtr udpConnection::available()
	{
		udpConnectionImpl *impl = (udpConnectionImpl*)this;
		return impl->available();
	}

	uintPtr udpConnection::read(void *buffer, uintPtr size, uint32 &channel, bool &reliable)
	{
		memoryBuffer b = read(channel, reliable);
		if (b.size() > size)
			CAGE_THROW_ERROR(exception, "buffer is too small");
		detail::memcpy(buffer, b.data(), b.size());
		return b.size();
	}

	uintPtr udpConnection::read(void *buffer, uintPtr size)
	{
		uint32 channel;
		bool reliable;
		return read(buffer, size, channel, reliable);
	}

	memoryBuffer udpConnection::read(uint32 &channel, bool &reliable)
	{
		udpConnectionImpl *impl = (udpConnectionImpl*)this;
		return impl->read(channel, reliable);
	}

	memoryBuffer udpConnection::read()
	{
		uint32 channel;
		bool reliable;
		return read(channel, reliable);
	}

	void udpConnection::write(const void *buffer, uintPtr size, uint32 channel, bool reliable)
	{
		memoryBuffer b(size);
		detail::memcpy(b.data(), buffer, size);
		write(b, channel, reliable);
	}

	void udpConnection::write(const memoryBuffer &buffer, uint32 channel, bool reliable)
	{
		udpConnectionImpl *impl = (udpConnectionImpl*)this;
		impl->write(buffer, channel, reliable);
	}

	void udpConnection::update()
	{
		udpConnectionImpl *impl = (udpConnectionImpl*)this;
		impl->service();
	}

	const udpConnectionStatistics &udpConnection::statistics() const
	{
		const udpConnectionImpl *impl = (const udpConnectionImpl*)this;
		return impl->stats;
	}

	holder<udpConnection> udpServer::accept()
	{
		udpServerImpl *impl = (udpServerImpl*)this;
		return impl->accept();
	}

	holder<udpConnection> newUdpConnection(const string &address, uint16 port, uint64 timeout)
	{
		return detail::systemArena().createImpl<udpConnection, udpConnectionImpl>(address, port, timeout);
	}

	holder<udpServer> newUdpServer(uint16 port)
	{
		return detail::systemArena().createImpl<udpServer, udpServerImpl>(port);
	}

	udpConnectionStatistics::udpConnectionStatistics()
	{
		detail::memset(this, 0, sizeof(*this));
	}
}
