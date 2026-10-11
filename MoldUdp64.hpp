#ifndef MOLD_UDP64_HPP
#define MOLD_UDP64_HPP

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

/*
    BISTECH MoldUDP64 transport

    A packet (one UDP datagram, or one record of a capture file) is

        session id  10 bytes
        sequence     8 bytes
        count        2 bytes
        then count messages, each: length (2 bytes) + message

    The sequence number is the sequence of the first message of the packet,
    a count of 0 is a heartbeat packet and carries no message.

    The connector delivers the messages of the feed in order, from a socket
    (multicast or unicast) or from a capture file. It does not look inside the
    messages; that is ItchDecoder's job.
*/

class MoldUdp64Connector {
public:
    static constexpr std::size_t SESSION_SIZE  = 10;
    static constexpr std::size_t SEQUENCE_SIZE = 8;
    static constexpr std::size_t COUNT_SIZE    = 2;
    static constexpr std::size_t HEADER_SIZE   = SESSION_SIZE + SEQUENCE_SIZE + COUNT_SIZE;
    static constexpr std::size_t LENGTH_SIZE   = 2;
    static constexpr std::size_t MAX_PACKET    = 65536;

    MoldUdp64Connector() = default;

    ~MoldUdp64Connector();

    MoldUdp64Connector(const MoldUdp64Connector&) = delete;
    MoldUdp64Connector& operator=(const MoldUdp64Connector&) = delete;

    // Replays the MoldUDP64 packets of a capture file.
    bool open_file(const std::string& path);

    // Receives UDP packets on a port. An empty group receives unicast,
    // otherwise the multicast group is joined on interface_address
    // (empty: the default interface).
    bool open_socket(std::uint16_t port, const std::string& group, const std::string& interface_address);

    void close();

    // Delivers the next message of the feed. Returns false when the feed ends
    // (end of file, or a socket error).
    bool next_message(const unsigned char*& data, std::size_t& length);

    std::uint64_t packets() const;
    std::uint64_t messages() const;
    std::uint64_t gaps() const;
    std::uint64_t missed() const;

    const std::string& session() const;

private:
    std::vector<unsigned char> packet_ = std::vector<unsigned char>(MAX_PACKET, 0);
    std::ifstream file_;
    int fd_ = -1;

    std::string session_;
    std::uint64_t expected_ = 0; // sequence of the next message expected
    std::uint64_t packets_ = 0;
    std::uint64_t messages_ = 0;
    std::uint64_t gaps_ = 0;
    std::uint64_t missed_ = 0;
    std::uint16_t count_ = 0;   // messages in the packet being consumed
    std::uint16_t message_ = 0; // messages already consumed
    std::size_t offset_ = 0;    // read position in the packet being consumed

    bool read_packet();
    bool read_file_packet();
    bool read_socket_packet();

    // Reads one length-prefixed message of a capture file into the packet.
    bool read_file_message(std::size_t& used);

    // Takes a parsed packet into consumption and follows the session sequence.
    bool start_packet(std::size_t bytes);
};

#endif
