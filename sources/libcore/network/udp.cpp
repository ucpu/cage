#include <array>
#include <vector>
#include <deque>
#include <set>
#include <map>
#include <memory>
#include <unordered_map>

#include "net.h"
#include <cage-core/math.h> // random
#include <cage-core/config.h>
#include <cage-core/concurrent.h>
#include <cage-core/utility/serialization.h>

namespace cage
{
	namespace
	{
		using namespace privat;

		configFloat simulatedPacketLoss("cage-core.udp.simulatedPacketLoss", 0);
		configUint32 mtu("cage-core.udp.mtu", 1100);
		configUint32 logLevel("cage-core.udp.logLevel", 0);
#define UDP_LOG(LEVEL, MSG) { if (logLevel >= (LEVEL)) { CAGE_LOG(severityEnum::Info, "udp", string() + MSG); } }

		struct sockGroupStruct
		{
			sockGroupStruct()
			{
				mut = newMutex();
			}

			struct receiverStruct
			{
				receiverStruct() : sockIndex(-1), connId(0)
				{}

				addr address;
				std::vector<memoryBuffer> packets;
				uint32 sockIndex;
				uint32 connId;
			};

			std::unordered_map<uint32, std::weak_ptr<receiverStruct>> receivers;
			std::weak_ptr<std::vector<std::shared_ptr<receiverStruct>>> accepting;
			std::vector<sock> socks;
			holder<mutexClass> mut;

			void readAll()
			{
				uint32 sockIndex = 0;
				for (sock &s : socks)
				{
					if (!s.isValid())
					{
						sockIndex++;
						continue;
					}
					while (true)
					{
						auto a = s.available();
						if (!a)
							break;
						memoryBuffer b(a);
						addr adr;
						auto rs = s.recvFrom(b.data(), a, adr, 0, true);
						if (rs >= 8)
						{
							uint32 connId = ((uint32*)b.data())[1];
							auto r = receivers[connId].lock();
							if (r)
							{
								r->packets.push_back(templates::move(b));
								if (r->sockIndex == -1)
								{
									r->sockIndex = sockIndex;
									r->address = adr;
								}
							}
							else
							{
								auto ac = accepting.lock();
								if (ac)
								{
									auto s = std::make_shared<receiverStruct>();
									s->address = adr;
									s->connId = connId;
									s->packets.push_back(templates::move(b));
									s->sockIndex = sockIndex;
									receivers[connId] = s;
									ac->push_back(s);
								}
							}
						}
						else
						{
							UDP_LOG(7, "received invalid packet of " + rs + " bytes, available " + a + " bytes");
						}
					}
					sockIndex++;
				}
				CAGE_ASSERT_RUNTIME(sockIndex == socks.size(), sockIndex, socks.size());
				for (auto it = receivers.begin(); it != receivers.end();)
				{
					if (!it->second.lock())
						it = receivers.erase(it);
					else
						it++;
				}
			}
		};

		// compare sequance numbers with correct wrapping
		// semantically: return a < b
		constexpr bool comp(uint16 a, uint16 b)
		{
			return (a < b && b - a < 32768)
				|| (a > b && a - b > 32768);
		}

		CAGE_ASSERT_COMPILE(comp(5, 10), compare_sequence_numbers);
		CAGE_ASSERT_COMPILE(comp(50000, 60000), compare_sequence_numbers);
		CAGE_ASSERT_COMPILE(comp(60000, 5), compare_sequence_numbers);
		CAGE_ASSERT_COMPILE(!comp(10, 5), compare_sequence_numbers);
		CAGE_ASSERT_COMPILE(!comp(60000, 50000), compare_sequence_numbers);
		CAGE_ASSERT_COMPILE(!comp(5, 60000), compare_sequence_numbers);
		CAGE_ASSERT_COMPILE(!comp(5, 5), compare_sequence_numbers);
		CAGE_ASSERT_COMPILE(!comp(60000, 60000), compare_sequence_numbers);

		class udpConnectionImpl : public udpConnectionClass
		{
		public:
			udpConnectionImpl(const string &address, uint16 port, uint64 timeout) : startTime(getApplicationTime()), connId(random(1u, detail::numeric_limits<uint32>::max())), established(false)
			{
				UDP_LOG(1, "creating new connection to address: '" + address + "', port: " + port + ", timeout: " + timeout);
				sockGroup = std::make_shared<sockGroupStruct>();
				addrList lst(address, port, AF_UNSPEC, SOCK_DGRAM, IPPROTO_UDP, 0);
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
							threadSleep(1000);
							service();
							if (established)
								break;
							if (getApplicationTime() > startTime + timeout)
								CAGE_THROW_ERROR(disconnectedException, "failed to connect (timeout)");
						}
					}
				}
			}

			udpConnectionImpl(std::shared_ptr<sockGroupStruct> sg, std::shared_ptr<sockGroupStruct::receiverStruct> rec) : sockGroup(sg), sockReceiver(rec), startTime(getApplicationTime()), connId(rec->connId), established(false)
			{
				UDP_LOG(1, "accepting new connection");
				serviceReceiving();
				if (!established)
					CAGE_THROW_ERROR(exception, "the received packet does not initialize a new connection");
			}

			~udpConnectionImpl()
			{
				UDP_LOG(2, "destroying connection");
			}

			// COMMAND

			enum class cmdType : uint8
			{
				invalid = 0,
				connectionInit, // uint16 index, 256 bytes connection identifier
				shortMessage, // uint16 msgSeqn, uint8 channel, uint16 size, data
				longMessageHead, // uint16 longSeqn, uint32 totalSize, uint16 msgSeqn, uint8 channel, 256 bytes data
				longMessageBody, // uint16 longSeqn, uint16 index, 256 bytes data
				longMessageTail, // uint16 longSeqn, uint16 size, data
			};

			struct commandStruct
			{
				std::array<char, 256> data;
				uint32 totalSize;
				uint16 size; // valid: 1 .. 256 inclusive
				uint16 msgSeqn;
				uint16 longSeqn;
				uint16 index;
				cmdType type;
				uint8 channel;

				commandStruct()
				{
					detail::memset(this, 0, sizeof(*this));
				}

				memoryBuffer serialize() const
				{
					memoryBuffer b;
					b.reserve(sizeof(commandStruct));
					serializer s(b);
					s << type;
					switch(type)
					{
					case cmdType::connectionInit:
						s << index;
						break;
					case cmdType::shortMessage:
						s << msgSeqn << channel << size;
						s.write(data.data(), size);
						break;
					case cmdType::longMessageHead:
						s << longSeqn << totalSize << msgSeqn << channel;
						s.write(data.data(), 256);
						break;
					case cmdType::longMessageBody:
						s << longSeqn << index;
						s.write(data.data(), 256);
						break;
					case cmdType::longMessageTail:
						s << longSeqn << size;
						s.write(data.data(), size);
						break;
					default:
						// critical - we should have had this under control
						CAGE_THROW_CRITICAL(exception, "invalid command type");
					}
					return b;
				}
			
				void deserialize(deserializer &d)
				{
					detail::memset(this, 0, sizeof(*this));
					d >> type;
					switch (type)
					{
					case cmdType::connectionInit:
						d >> index;
						break;
					case cmdType::shortMessage:
						d >> msgSeqn >> channel >> size;
						d.read(data.data(), size);
						break;
					case cmdType::longMessageHead:
						d >> longSeqn >> totalSize >> msgSeqn >> channel;
						d.read(data.data(), 256);
						size = 256;
						break;
					case cmdType::longMessageBody:
						d >> longSeqn >> index;
						d.read(data.data(), 256);
						size = 256;
						break;
					case cmdType::longMessageTail:
						d >> longSeqn >> size;
						d.read(data.data(), size);
						index = (uint16)-1;
						break;
					default:
						// error - the packet could get corrupted on the network
						CAGE_THROW_ERROR(exception, "invalid command type");
					}
				}
			};

			// SENDING

			struct sendingStruct
			{
				struct sendingCommandStruct : public commandStruct
				{
					uint64 lastTimeSend;
					uint32 sendCount;

					sendingCommandStruct(const commandStruct &cmd) : commandStruct(cmd), lastTimeSend(0), sendCount(0)
					{}
				};

				std::array<uint16, 256> seqnPerChannel; // next seqn to be used
				uint16 packetSeqn; // next seqn to be used
				uint16 longSeqn; // next seqn to be used
				std::deque<std::shared_ptr<sendingCommandStruct>> cmds;
				std::set<std::shared_ptr<sendingCommandStruct>> resendCmds;
				std::map<uint16, std::vector<std::weak_ptr<sendingCommandStruct>>> resendsInPackets;

				sendingStruct() : packetSeqn(0), longSeqn(0)
				{
					detail::memset(seqnPerChannel.data(), 0, 256 * 2);
				}

				void addNewCmd(const commandStruct &cmd)
				{
					auto cmdp = std::make_shared<sendingCommandStruct>(cmd);
					if (cmdp->channel >= 128)
						resendCmds.insert(cmdp);
					else
						cmds.push_back(cmdp);
				}

				void ack(uint16 packetSeqn) // packet seqn
				{
					for (auto &w : resendsInPackets[packetSeqn])
					{
						auto c = w.lock();
						if (!c)
							continue;
						c->type = cmdType::invalid;
						resendCmds.erase(c);
					}
					resendsInPackets.erase(packetSeqn);
				}
			} sending;

			memoryBuffer makePacket()
			{
				if (sending.cmds.empty())
					return {};
				uint64 current = getApplicationTime();
				memoryBuffer compressed;
				memoryBuffer working;
				serializer s(working);
				uint32 ackBits = receiving.makeAckBits();
				s << sending.packetSeqn << receiving.packetSeqn << ackBits;
				UDP_LOG(3, "preparing packet seqn " + sending.packetSeqn + ", ack-seqn " + receiving.packetSeqn + ", ack-bits: " + ackBits);
				uint32 m = mtu;
				while (!sending.cmds.empty())
				{
					auto cmdp = sending.cmds.front();
					if (cmdp->type == cmdType::invalid)
					{
						sending.cmds.pop_front();
						continue;
					}
					memoryBuffer cmd = cmdp->serialize();
					s.write(cmd.data(), cmd.size());
					memoryBuffer tmp = detail::compress(working);
					//memoryBuffer tmp = working.copy();
					if (tmp.size() > m)
						break;
					compressed = templates::move(tmp);
					sending.cmds.pop_front();
					cmdp->lastTimeSend = current;
					cmdp->sendCount++;
					if (cmdp->channel >= 128)
						sending.resendsInPackets[sending.packetSeqn].push_back(cmdp);
					UDP_LOG(5, "added command of type " + (uint32)cmdp->type + ", msg-seqn: " + cmdp->msgSeqn + ", channel: " + cmdp->channel + ", long-seqn: " + cmdp->longSeqn + ", long-index: " + cmdp->index + ", size: " + cmdp->size);
				}
				sending.packetSeqn++;
				memoryBuffer result(compressed.size() + 8);
				detail::memcpy(result.data(), "cage", 4);
				detail::memcpy(result.data() + 4, &connId, 4);
				detail::memcpy(result.data() + 8, compressed.data(), compressed.size());
				UDP_LOG(4, "prepared packet with original size: " + working.size() + ", compressed size: " + compressed.size());
				return result;
			}

			void serviceSending()
			{
				if (!established)
				{
					commandStruct cmd;
					cmd.type = cmdType::connectionInit;
					sending.addNewCmd(cmd);
				}

				{ // resends
					uint64 current = getApplicationTime();
					for (auto it = sending.resendCmds.begin(); it != sending.resendCmds.end(); )
					{
						auto &cmd = *it;
						if (cmd->type == cmdType::invalid)
						{
							it = sending.resendCmds.erase(it);
							continue;
						}
						if ((cmd->lastTimeSend + (1u << cmd->sendCount) * 10000) < current)
							sending.cmds.push_back(cmd);
						it++;
					}
				}

				// make packets and send them
				while (true)
				{
					memoryBuffer b = makePacket();
					if (b.size() == 0)
						break;

					if (random() < real(simulatedPacketLoss))
					{
						UDP_LOG(4, "droppping prepared packet due to simulated packet loss");
						continue;
					}

					// sending does not need to be under mutex
					if (sockReceiver->sockIndex == -1)
					{
						for (sock &s : sockGroup->socks)
						{
							if (!s.isValid())
								continue;
							try
							{
								s.send(b.data(), b.size());
							}
							catch (...)
							{
								// do nothing, i guess
							}
						}
					}
					else
					{
						sock &s = sockGroup->socks[sockReceiver->sockIndex];
						if (s.isValid())
						{
							try
							{
								s.sendTo(b.data(), b.size(), sockReceiver->address);
							}
							catch (...)
							{
								// do nothing, i guess
							}
						}
					}
				}
			}

			// RECEIVING

			struct receivingStruct
			{
				std::array<uint16, 256> seqnPerChannel; // minimum expected seqn
				std::set<uint16> receivedPacketsSeqns;
				uint16 packetSeqn; // minimum expected seqn

				struct longStruct
				{
					struct partStruct
					{
						std::array<char, 256> data;
					};
					std::map<uint16, partStruct> parts;
					uint32 totalSize;
					uint16 msgSeqn;
					uint8 channel;
					
					longStruct() : totalSize(0), msgSeqn(0), channel(0)
					{}

					bool isComplete() const
					{
						if (totalSize == 0)
							return false;
						uint32 cnt = totalSize / 256 + (totalSize % 256 > 0 ? 1 : 0);
						return parts.size() == cnt;
					}
				};
				std::map<uint16, longStruct> longs;

				struct finalStruct
				{
					memoryBuffer data;
					uint16 msgSeqn;
					uint8 channel;
					finalStruct() : channel(0), msgSeqn(0)
					{}
				};
				std::array<std::vector<finalStruct>, 256> holds;
				std::deque<finalStruct> messages;

				receivingStruct() : packetSeqn(0)
				{
					detail::memset(seqnPerChannel.data(), 0, 256 * 2);
				}

				void handleReceivedMessage(finalStruct &&msg)
				{
					holds[msg.channel].push_back(templates::move(msg));
				}

				void consolidate()
				{
					{ // check for completed long messages
						std::vector<uint16> completedLongs;
						for (auto &p : longs)
						{
							if (!p.second.isComplete())
								continue;
							completedLongs.push_back(p.first);
							finalStruct f;
							f.channel = p.second.channel;
							f.msgSeqn = p.second.msgSeqn;
							f.data.reallocate(p.second.totalSize);
							uint32 maxIndex = p.second.totalSize / 256;
							for (auto &d : p.second.parts)
							{
								if (d.first == (uint16)-1)
								{ // tail
									uint32 off = (p.second.totalSize / 256) * 256;
									detail::memcpy(f.data.data() + off, d.second.data.data(), p.second.totalSize - off);
								}
								else
								{ // body (or head)
									if (d.first > maxIndex)
										CAGE_THROW_ERROR(exception, "long-message body-part-index out of range");
									detail::memcpy(f.data.data() + d.first * 256, d.second.data.data(), 256);
								}
							}
							handleReceivedMessage(templates::move(f));
						}
						for (uint16 it : completedLongs)
							longs.erase(it);
					}

					// todo delete too old (non-reliable) long messages

					{ // check messages on hold
						// sort all messages (per channel)
						for (uint32 ch = 0; ch < 256; ch++)
						{
							std::sort(holds[ch].begin(), holds[ch].end(), [](const finalStruct &a, const finalStruct &b) {
								return comp(a.msgSeqn, b.msgSeqn);
							});
						}

						// non-reliable messages
						for (uint32 ch = 0; ch < 128; ch++)
						{
							for (finalStruct &msg : holds[ch])
							{
								if (!comp(msg.msgSeqn, seqnPerChannel[ch]))
								{
									seqnPerChannel[ch] = msg.msgSeqn + (uint16)1;
									messages.push_back(templates::move(msg));
								}
							}
							holds[ch].clear();
						}

						// reliable messages
						for (uint32 ch = 128; ch < 256; ch++)
						{
							holds[ch].erase(std::remove_if(holds[ch].begin(), holds[ch].end(), [&](finalStruct &msg) {
								if (comp(msg.msgSeqn, seqnPerChannel[ch]))
									return true;
								if (msg.msgSeqn == seqnPerChannel[ch])
								{
									seqnPerChannel[ch] = msg.msgSeqn + (uint16)1;
									messages.push_back(templates::move(msg));
									return true;
								}
								return false;
							}), holds[ch].end());
						}
					}
				}

				uint32 makeAckBits()
				{
					uint32 result = 0;
					for (uint32 i = 0; i < 32; i++)
					{
						uint16 s = packetSeqn - i;
						if (receivedPacketsSeqns.count(s))
						{
							uint32 m = 1 << i;
							result |= m;
						}
					}
					return result;
				}
			} receiving;

			void handleReceivedCommand(const commandStruct &cmd)
			{
				switch (cmd.type)
				{
				case cmdType::connectionInit:
				{
					UDP_LOG(3, "received connection init command with index " + cmd.index);
					if (!established)
					{
						if (logLevel >= 2)
						{
							string a;
							uint16 p;
							sockReceiver->address.translate(a, p, true);
							CAGE_LOG(severityEnum::Info, "udp", string() + "connection established, remote address: \"" + a + "\", remote port: " + p);
						}
						established = true;
					}
					if (cmd.index == 0)
					{
						commandStruct c = cmd;
						c.index = 1;
						sending.addNewCmd(c);
					}
				} break;
				case cmdType::shortMessage:
				{
					UDP_LOG(4, "received short message on channel " + cmd.channel + ", seqn " + cmd.msgSeqn + " and " + cmd.size + " bytes of data");
					receivingStruct::finalStruct msg;
					msg.data.reallocate(cmd.size);
					detail::memcpy(msg.data.data(), cmd.data.data(), cmd.size);
					msg.channel = cmd.channel;
					msg.msgSeqn = cmd.msgSeqn;
					receiving.handleReceivedMessage(std::move(msg));
				} break;
				case cmdType::longMessageHead:
				{
					UDP_LOG(4, "received long message head with id " + cmd.longSeqn + ", channel " + cmd.channel + ", seqn " + cmd.msgSeqn + " and " + cmd.totalSize + " bytes of data");
					receivingStruct::longStruct &l = receiving.longs[cmd.longSeqn];
					l.parts[0].data = cmd.data;
					l.totalSize = cmd.totalSize;
					l.channel = cmd.channel;
					l.msgSeqn = cmd.msgSeqn;
				} break;
				case cmdType::longMessageBody:
				case cmdType::longMessageTail:
				{
					UDP_LOG(5, "received long message body with id " + cmd.longSeqn + ", index " + cmd.index);
					receivingStruct::longStruct &l = receiving.longs[cmd.longSeqn];
					l.parts[cmd.index].data = cmd.data;
				} break;
				default:
					// critical - the type was already checked during deserialization
					CAGE_THROW_CRITICAL(exception, "invalid command type");
				}
			}

			void handleReceivedPacket(const memoryBuffer &wholePacket)
			{
				UDP_LOG(5, "received packet of " + wholePacket.size() + " bytes");
				memoryBuffer b;
				{
					// decompress packet
					deserializer d(wholePacket);
					{
						// read signature
						char c, a, g, e;
						d >> c >> a >> g >> e;
						if (c != 'c' || a != 'a' || g != 'g' || e != 'e')
							CAGE_THROW_ERROR(exception, "invalid udp packet signature");
						// read connection id
						uint32 id;
						d >> id;
					}
					memoryBuffer tmp(wholePacket.size() - 8);
					detail::memcpy(tmp.data(), wholePacket.data() + 8, tmp.size());
					b = detail::decompress(tmp, wholePacket.size() * 10);
					//b = templates::move(tmp);
				}
				deserializer d(b);
				{
					// read packet header
					uint16 packetSeqn;
					uint16 ackSeqn;
					uint32 ackBits;
					d >> packetSeqn >> ackSeqn >> ackBits;
					UDP_LOG(3, "received packet seqn " + packetSeqn + ", ack-seqn " + ackSeqn + ", ack-bits " + ackBits + ", decompressed size " + b.size() + " bytes");
					// check if the packet is in order
					if (comp(packetSeqn, receiving.packetSeqn))
					{
						UDP_LOG(4, "the packet is out of order");
						// the packet is a duplicate or was delayed
						// since packet duplication or delay are common on network, we do not want to close the socket
						//   therefore we ignore the packet, but do not throw an exception
						return;
					}
					receiving.packetSeqn = packetSeqn + (uint16)1;
					receiving.receivedPacketsSeqns.insert(packetSeqn);
					// process acks
					for (uint32 i = 0; i < 32; i++)
					{
						uint32 m = 1 << i;
						if ((ackBits & m) == m)
						{
							uint16 s = ackSeqn - i;
							sending.ack(s);
						}
					}
				}
				// read packet commands
				while (d.current < d.end)
				{
					commandStruct cmd;
					cmd.deserialize(d);
					handleReceivedCommand(cmd);
				}
			}

			void serviceReceiving()
			{
				std::vector<memoryBuffer> packets;
				{
					scopeLock<mutexClass> lock(sockGroup->mut);
					sockGroup->readAll();
					sockReceiver->packets.swap(packets);
				}
				for (memoryBuffer &b : packets)
					handleReceivedPacket(b);
				receiving.consolidate();
			}

			// COMMON

			std::shared_ptr<sockGroupStruct> sockGroup;
			std::shared_ptr<sockGroupStruct::receiverStruct> sockReceiver;
			const uint64 startTime;
			const uint32 connId;
			bool established;

			uintPtr available()
			{
				detail::overrideBreakpoint brk;
				if (receiving.messages.empty())
					serviceReceiving();
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
				CAGE_ASSERT_RUNTIME(channel < 128, channel, reliable);
				CAGE_ASSERT_RUNTIME(buffer.size() <= 16 * 1024 * 1024, buffer.size(), channel, reliable);
				if (buffer.size() == 0)
					return; // ignore empty messages

				// common command fields
				commandStruct cmd;
				cmd.channel = numeric_cast<uint8>(channel + reliable * 128);
				cmd.msgSeqn = sending.seqnPerChannel[cmd.channel]++;
				cmd.totalSize = numeric_cast<uint32>(buffer.size());

				if (buffer.size() > 256)
				{ // long message
					cmd.longSeqn = sending.longSeqn++;
					cmd.size = 256;
					// head
					cmd.type = cmdType::longMessageHead;
					detail::memcpy(cmd.data.data(), buffer.data(), 256);
					sending.addNewCmd(cmd);
					// bodies
					cmd.type = cmdType::longMessageBody;
					uint32 bodies = numeric_cast<uint32>(buffer.size() / 256);
					for (uint32 i = 1; i < bodies; i++)
					{
						cmd.index = i;
						detail::memcpy(cmd.data.data(), buffer.data() + i * 256, 256);
						sending.addNewCmd(cmd);
					}
					// tail
					CAGE_ASSERT_RUNTIME(buffer.size() - bodies * 256 <= 256, buffer.size(), bodies);
					if (buffer.size() > bodies * 256)
					{
						cmd.type = cmdType::longMessageTail;
						cmd.size = numeric_cast<uint16>(buffer.size() - bodies * 256);
						detail::memcpy(cmd.data.data(), buffer.data() + bodies * 256, cmd.size);
						sending.addNewCmd(cmd);
					}
				}
				else
				{ // short message
					cmd.type = cmdType::shortMessage;
					cmd.size = numeric_cast<uint16>(buffer.size());
					detail::memcpy(cmd.data.data(), buffer.data(), buffer.size());
					sending.addNewCmd(cmd);
				}
			}

			void service()
			{
				detail::overrideBreakpoint brk;

				serviceReceiving();
				serviceSending();

				if (logLevel >= 6)
				{
					if (sending.resendCmds.size())
						CAGE_LOG(severityEnum::Info, "udp", string() + "unacknowledged commands count: " + sending.resendCmds.size());
					if (receiving.longs.size())
						CAGE_LOG(severityEnum::Info, "udp", string() + "incomplete long messages count: " + receiving.longs.size());
					uint32 onhold = 0;
					for (auto &i : receiving.holds)
						onhold += numeric_cast<uint32>(i.size());
					if (onhold)
						CAGE_LOG(severityEnum::Info, "udp", string() + "on hold messages count: " + onhold);
				}
			}
		};

		class udpServerImpl : public udpServerClass
		{
		public:
			udpServerImpl(uint16 port)
			{
				UDP_LOG(1, "creating new server on port " + port);
				sockGroup = std::make_shared<sockGroupStruct>();
				addrList lst(nullptr, port, AF_UNSPEC, SOCK_DGRAM, IPPROTO_UDP, 0);
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

			holder<udpConnectionClass> accept()
			{
				detail::overrideBreakpoint brk;
				std::shared_ptr<sockGroupStruct::receiverStruct> acc;
				{
					scopeLock<mutexClass> lock(sockGroup->mut);
					sockGroup->readAll();
					if (accepting->empty())
						return {};
					acc = accepting->back();
					accepting->pop_back();
				}
				try
				{
					return detail::systemArena().createImpl<udpConnectionClass, udpConnectionImpl>(sockGroup, acc);
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

	uintPtr udpConnectionClass::available()
	{
		udpConnectionImpl *impl = (udpConnectionImpl*)this;
		return impl->available();
	}

	uintPtr udpConnectionClass::read(void *buffer, uintPtr size, uint32 &channel, bool &reliable)
	{
		memoryBuffer b = read(channel, reliable);
		if (b.size() > size)
			CAGE_THROW_ERROR(exception, "buffer is too small");
		detail::memcpy(buffer, b.data(), b.size());
		return b.size();
	}

	uintPtr udpConnectionClass::read(void *buffer, uintPtr size)
	{
		uint32 channel;
		bool reliable;
		return read(buffer, size, channel, reliable);
	}

	memoryBuffer udpConnectionClass::read(uint32 &channel, bool &reliable)
	{
		udpConnectionImpl *impl = (udpConnectionImpl*)this;
		return impl->read(channel, reliable);
	}

	memoryBuffer udpConnectionClass::read()
	{
		uint32 channel;
		bool reliable;
		return read(channel, reliable);
	}

	void udpConnectionClass::write(const void *buffer, uintPtr size, uint32 channel, bool reliable)
	{
		memoryBuffer b(size);
		detail::memcpy(b.data(), buffer, size);
		write(b, channel, reliable);
	}

	void udpConnectionClass::write(const memoryBuffer &buffer, uint32 channel, bool reliable)
	{
		udpConnectionImpl *impl = (udpConnectionImpl*)this;
		impl->write(buffer, channel, reliable);
	}

	void udpConnectionClass::update()
	{
		udpConnectionImpl *impl = (udpConnectionImpl*)this;
		impl->service();
	}

	holder<udpConnectionClass> udpServerClass::accept()
	{
		udpServerImpl *impl = (udpServerImpl*)this;
		return impl->accept();
	}

	holder<udpConnectionClass> newUdpConnection(const string &address, uint16 port, uint64 timeout)
	{
		return detail::systemArena().createImpl<udpConnectionClass, udpConnectionImpl>(address, port, timeout);
	}

	holder<udpServerClass> newUdpServer(uint16 port)
	{
		return detail::systemArena().createImpl<udpServerClass, udpServerImpl>(port);
	}
}
