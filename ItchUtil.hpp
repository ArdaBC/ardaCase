#ifndef ITCH_UTIL_HPP
#define ITCH_UTIL_HPP

#include <cstdint>
#include <cstddef>

/*
    BISTECH ITCH v2112 message formats

    T  Timestamp Seconds          5 bytes
    R  Order Book Directory      130 bytes
    M  Combination Order Book    18 bytes
    L  Tick Size Table Entry     25 bytes
    V  Short Sell Status         10 bytes
    S  System Event               6 bytes
    O  Order Book State          29 bytes

    A  Add Order                 45 bytes
    F  Add Order MPID            44 bytes  (not used by BIST)
    E  Order Executed            52 bytes
    C  Order Executed with Price 58 bytes
    U  Order Replace             36 bytes  (not used by BIST)
    D  Order Delete              18 bytes
    Y  Order Book Flush           9 bytes

    P  Trade                     50 bytes
    Z  Equilibrium Price Update  53 bytes

    All numeric fields are big-endian.
*/

enum class EventType {
    Add,
    Execute,
    ExecuteWithPrice,
    Cancel,
    Delete,
    Replace,
    Flush,
    Unknown
};

enum class Side {
    Buy,
    Sell
};

struct Event {
    EventType type = EventType::Unknown;
    std::uint32_t order_book_id = 0;
    std::uint64_t order_ref = 0;
    std::uint64_t new_order_ref = 0;
    std::uint64_t shares = 0;
    std::int32_t price = 0;
    Side side = Side::Buy;
    std::uint64_t timestamp = 0;
};

class ItchUtil {
public:
    ItchUtil() = delete;
    ~ItchUtil() = delete;
    ItchUtil(const ItchUtil&) = delete;
    ItchUtil& operator=(const ItchUtil&) = delete;

    static uint16_t read_uint16(const unsigned char* p) {
        return
            (uint16_t(p[0]) << 8) |
            (uint16_t(p[1]));
    }

    static uint32_t read_uint32(const unsigned char* p) {
        return
            (uint32_t(p[0]) << 24) |
            (uint32_t(p[1]) << 16) |
            (uint32_t(p[2]) << 8) |
            uint32_t(p[3]);
    }

    static uint64_t read_uint64(const unsigned char* p) {
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

    static int32_t read_int32(const unsigned char* p) {
        return static_cast<int32_t>(read_uint32(p));
    }
};

#endif