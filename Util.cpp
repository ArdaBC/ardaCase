#include "Util.hpp"

std::uint16_t Util::read_uint16(const unsigned char* p) noexcept {
    return
        (uint16_t(p[0]) << 8) |
        (uint16_t(p[1]));
}

std::uint32_t Util::read_uint32(const unsigned char* p) noexcept {
    return
        (uint32_t(p[0]) << 24) |
        (uint32_t(p[1]) << 16) |
        (uint32_t(p[2]) << 8) |
        uint32_t(p[3]);
}

std::uint64_t Util::read_uint64(const unsigned char* p) noexcept {
    return
        (uint64_t(p[0]) << 56) |
        (uint64_t(p[1]) << 48) |
        (uint64_t(p[2]) << 40) |
        (uint64_t(p[3]) << 32) |
        (uint64_t(p[4]) << 24) |
        (uint64_t(p[5]) << 16) |
        (uint64_t(p[6]) << 8) |
        uint64_t(p[7]);
}

std::int32_t Util::read_int32(const unsigned char* p) noexcept {
    return static_cast<int32_t>(read_uint32(p));
}

bool Util::read_side(unsigned char value, Side& side) noexcept {
    if (value == 'B') {
        side = Side::Buy;
        return true;
    }

    if (value == 'S') {
        side = Side::Sell;
        return true;
    }

    return false;
}

std::uint64_t Util::make_timestamp(std::uint32_t seconds, std::uint32_t nanoseconds) noexcept {
    return static_cast<std::uint64_t>(seconds) * 1000000000ULL + nanoseconds;
}

bool Util::read_order_prefix(const unsigned char* data, std::uint32_t seconds, OrderPrefix& prefix) noexcept {
    prefix.timestamp = make_timestamp(seconds, read_uint32(data + 1));
    prefix.order_ref = read_uint64(data + 5);
    prefix.order_book_id = read_uint32(data + 13);

    return read_side(data[17], prefix.side);
}
