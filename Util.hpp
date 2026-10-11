#ifndef UTIL_HPP
#define UTIL_HPP

#include <cstddef>
#include <cstdint>

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

// The fields every order message (A / E / C / D / U) begins with.
struct OrderPrefix {
    std::uint64_t timestamp = 0; // seconds * 1e9 + nanoseconds
    std::uint64_t order_ref = 0;
    std::uint32_t order_book_id = 0;
    Side side = Side::Buy;
};

// Big-endian field reads and the few ITCH fields shared by the decoder,
// the connector and the order book.
class Util {
public:
    Util() = delete;
    ~Util() = delete;
    Util(const Util&) = delete;
    Util& operator=(const Util&) = delete;

    static std::uint16_t read_uint16(const unsigned char* p) noexcept;
    static std::uint32_t read_uint32(const unsigned char* p) noexcept;
    static std::uint64_t read_uint64(const unsigned char* p) noexcept;
    static std::int32_t read_int32(const unsigned char* p) noexcept;

    // Order message side: 'B' buy, 'S' sell.
    static bool read_side(unsigned char value, Side& side) noexcept;

    // A message carries the nanoseconds within the second, the 'T' message
    // carries the second itself.
    static std::uint64_t make_timestamp(std::uint32_t seconds, std::uint32_t nanoseconds) noexcept;

    // Reads the prefix all order messages share (timestamp, order reference,
    // order book id, side). The caller must have checked the message is at
    // least 18 bytes long, the side byte being the last of them.
    static bool read_order_prefix(const unsigned char* data, std::uint32_t seconds, OrderPrefix& prefix) noexcept;
};

#endif
