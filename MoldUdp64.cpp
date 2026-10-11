#include <cerrno>
#include <cstring>
#include <iostream>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include "MoldUdp64.hpp"
#include "Util.hpp"

MoldUdp64Connector::~MoldUdp64Connector() {
    close();
}

bool MoldUdp64Connector::open_file(const std::string& path) {
    close();

    file_.open(path, std::ios::binary);

    if (!file_.is_open()) {
        std::cerr << "Cannot open: " << path << '\n';
        return false;
    }

    return true;
}

bool MoldUdp64Connector::open_socket(std::uint16_t port, const std::string& group, const std::string& interface_address) {
    close();

    fd_ = ::socket(AF_INET, SOCK_DGRAM, 0);

    if (fd_ < 0) {
        std::cerr << "Cannot create socket: " << std::strerror(errno) << '\n';
        return false;
    }

    const int reuse = 1;
    ::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = htonl(INADDR_ANY);

    if (::bind(fd_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) {
        std::cerr << "Cannot bind UDP port " << port << ": " << std::strerror(errno) << '\n';
        close();
        return false;
    }

    if (group.empty()) {
        return true;
    }

    ip_mreq request{};
    request.imr_interface.s_addr = htonl(INADDR_ANY);

    const bool bad_group = ::inet_pton(AF_INET, group.c_str(), &request.imr_multiaddr) != 1;
    const bool bad_interface = !interface_address.empty() && ::inet_pton(AF_INET, interface_address.c_str(), &request.imr_interface) != 1;

    if (bad_group || bad_interface) {
        std::cerr << "Bad multicast address\n";
        close();
        return false;
    }

    if (::setsockopt(fd_, IPPROTO_IP, IP_ADD_MEMBERSHIP, &request, sizeof(request)) != 0) {
        std::cerr << "Cannot join " << group << ": " << std::strerror(errno) << '\n';
        close();
        return false;
    }

    return true;
}

void MoldUdp64Connector::close() {
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }

    if (file_.is_open()) {
        file_.close();
    }

    session_.clear();
    expected_ = 0;
    count_ = 0;
    message_ = 0;
    offset_ = 0;
    packets_ = 0;
    messages_ = 0;
    gaps_ = 0;
    missed_ = 0;
}

bool MoldUdp64Connector::next_message(const unsigned char*& data, std::size_t& length) {
    while (message_ >= count_) {
        if (!read_packet()) {
            return false;
        }
    }

    length = Util::read_uint16(packet_.data() + offset_);
    data = packet_.data() + offset_ + LENGTH_SIZE;
    offset_ += LENGTH_SIZE + length;
    ++message_;
    ++messages_;

    return true;
}

std::uint64_t MoldUdp64Connector::packets() const {
    return packets_;
}

std::uint64_t MoldUdp64Connector::messages() const {
    return messages_;
}

std::uint64_t MoldUdp64Connector::gaps() const {
    return gaps_;
}

std::uint64_t MoldUdp64Connector::missed() const {
    return missed_;
}

const std::string& MoldUdp64Connector::session() const {
    return session_;
}

bool MoldUdp64Connector::read_packet() {
    if (file_.is_open()) {
        return read_file_packet();
    }

    if (fd_ >= 0) {
        return read_socket_packet();
    }

    return false;
}

bool MoldUdp64Connector::read_file_message(std::size_t& used) {
    unsigned char length_bytes[LENGTH_SIZE];

    if (!file_.read(reinterpret_cast<char*>(length_bytes), LENGTH_SIZE)) {
        std::cerr << "Truncated packet\n";
        return false;
    }

    const std::uint16_t length = Util::read_uint16(length_bytes);

    if (used + LENGTH_SIZE + length > packet_.size()) {
        std::cerr << "Packet too large\n";
        return false;
    }

    packet_[used] = length_bytes[0];
    packet_[used + 1] = length_bytes[1];

    if (!file_.read(reinterpret_cast<char*>(packet_.data() + used + LENGTH_SIZE), length)) {
        std::cerr << "Truncated packet\n";
        return false;
    }

    used += LENGTH_SIZE + length;

    return true;
}

bool MoldUdp64Connector::read_file_packet() {
    if (!file_.read(reinterpret_cast<char*>(packet_.data()), HEADER_SIZE)) {
        return false;
    }

    const std::uint16_t count = Util::read_uint16(packet_.data() + SESSION_SIZE + SEQUENCE_SIZE);

    std::size_t used = HEADER_SIZE;

    for (std::uint16_t i = 0; i < count; ++i) {
        if (!read_file_message(used)) {
            return false;
        }
    }

    return start_packet(used);
}

bool MoldUdp64Connector::read_socket_packet() {
    while (true) {
        const ssize_t received = ::recvfrom(fd_, packet_.data(), packet_.size(), 0, nullptr, nullptr);

        if (received < 0) {
            if (errno == EINTR) {
                continue;
            }

            std::cerr << "Receive failed: " << std::strerror(errno) << '\n';
            return false;
        }

        if (static_cast<std::size_t>(received) < HEADER_SIZE) {
            std::cerr << "Short packet: " << received << " bytes\n";
            continue;
        }

        if (start_packet(static_cast<std::size_t>(received))) {
            return true;
        }
    }
}

bool MoldUdp64Connector::start_packet(std::size_t bytes) {
    const std::uint16_t count = Util::read_uint16(packet_.data() + HEADER_SIZE - COUNT_SIZE);
    const std::uint64_t sequence = Util::read_uint64(packet_.data() + SESSION_SIZE);

    std::size_t offset = HEADER_SIZE;

    for (std::uint16_t i = 0; i < count; ++i) {
        if (offset + LENGTH_SIZE > bytes) {
            std::cerr << "Malformed packet\n";
            return false;
        }

        offset += LENGTH_SIZE + Util::read_uint16(packet_.data() + offset);

        if (offset > bytes) {
            std::cerr << "Malformed packet\n";
            return false;
        }
    }

    const bool new_session = session_.size() != SESSION_SIZE || std::memcmp(session_.data(), packet_.data(), SESSION_SIZE) != 0;

    if (new_session) {
        session_.assign(reinterpret_cast<const char*>(packet_.data()), SESSION_SIZE);
        expected_ = sequence;
    }

    if (count > 0 && sequence != expected_) {
        ++gaps_;

        if (sequence > expected_) {
            missed_ += sequence - expected_;
        }

        std::cerr << "gap in " << session_ << ": expected " << expected_ << ", got " << sequence << '\n';
    }

    expected_ = sequence + count;
    count_ = count;
    message_ = 0;
    offset_ = HEADER_SIZE;
    ++packets_;

    return true;
}
