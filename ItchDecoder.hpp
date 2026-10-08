#ifndef ITCH_DECODER_HPP
#define ITCH_DECODER_HPP

#include <cstddef>
#include <cstdint>
#include "ItchUtil.hpp"

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

class ItchDecoder {
public:
    ItchDecoder() = default;

    bool decode(const unsigned char* data, std::size_t length, Event& event) noexcept {
        if (data == nullptr || length == 0) {
            return false;
        }

        const char type = static_cast<char>(data[0]);

        // Timestamp message: 'T'
        if (type == 'T') {
            if (length < 5)
                return false;

            timestamp_seconds_ = ItchUtil::read_uint32(data + 1);

            return false; // No Event generated
        }

        switch (type) {
            case 'A':
                return decode_add(data, length, event);

            case 'E':
                return decode_execute(data, length, event);

            case 'C':
                return decode_execute_with_price(data, length, event);

            case 'D':
                return decode_delete(data, length, event);

            case 'U':
                return decode_replace(data, length, event);

            case 'Y':
                return decode_flush(data, length, event);

            default:
                return false;
        }
    }

    void reset() noexcept {
        timestamp_seconds_ = 0;
    }

private:
    static constexpr std::size_t ADD_SIZE     = 45;
    static constexpr std::size_t EXEC_SIZE    = 52;
    static constexpr std::size_t EXEC_P_SIZE  = 58;
    static constexpr std::size_t DELETE_SIZE  = 18;
    static constexpr std::size_t REPLACE_SIZE = 36;
    static constexpr std::size_t FLUSH_SIZE   = 9;

    std::uint32_t timestamp_seconds_ = 0;

    static bool read_side(unsigned char value, Side& side) noexcept {
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

    std::uint64_t timestamp(const unsigned char* data) const noexcept {
        const std::uint32_t ns = ItchUtil::read_uint32(data + 1);

        return static_cast<std::uint64_t>(timestamp_seconds_) * 1000000000ULL + ns;
    }

    bool decode_add(const unsigned char* data, std::size_t length, Event& event) const noexcept {
        if (length < ADD_SIZE) {
            return false;
        }

        event = {};
        event.type = EventType::Add;

        event.timestamp = timestamp(data);

        event.order_ref = ItchUtil::read_uint64(data + 5);

        event.order_book_id = ItchUtil::read_uint32(data + 13);

        if (!read_side(data[17], event.side)) {
            return false;
        }
            
        event.shares = ItchUtil::read_uint64(data + 22);

        event.price = ItchUtil::read_int32(data + 30);

        return true;
    }

    bool decode_execute(const unsigned char* data, std::size_t length, Event& event) const noexcept {
        if (length < EXEC_SIZE) {
            return false;
        }

        event = {};
        event.type = EventType::Execute;

        event.timestamp = timestamp(data);

        event.order_ref = ItchUtil::read_uint64(data + 5);

        event.order_book_id = ItchUtil::read_uint32(data + 13);

        if (!read_side(data[17], event.side)) {
            return false;
        }

        event.shares = ItchUtil::read_uint64(data + 18);

        return true;
    }

    bool decode_execute_with_price(const unsigned char* data, std::size_t length, Event& event) const noexcept {
        if (length < EXEC_P_SIZE) {
            return false;
        }
            
        event = {};
        event.type = EventType::ExecuteWithPrice;

        event.timestamp = timestamp(data);

        event.order_ref = ItchUtil::read_uint64(data + 5);

        event.order_book_id = ItchUtil::read_uint32(data + 13);

        if (!read_side(data[17], event.side)) {
            return false;
        }

        event.shares = ItchUtil::read_uint64(data + 18);

        event.price = ItchUtil::read_int32(data + 52);

        return true;
    }

    bool decode_delete(const unsigned char* data, std::size_t length, Event& event) const noexcept {
        if (length < DELETE_SIZE) {
            return false;
        }
            
        event = {};
        event.type = EventType::Delete;

        event.timestamp = timestamp(data);

        event.order_ref = ItchUtil::read_uint64(data + 5);

        event.order_book_id = ItchUtil::read_uint32(data + 13);

        if (!read_side(data[17], event.side)) {
            return false;
        }

        return true;
    }

    bool decode_replace(const unsigned char* data, std::size_t length, Event& event) const noexcept {
        if (length < REPLACE_SIZE) {
            return false;
        }
            
        event = {};
        event.type = EventType::Replace;

        event.timestamp = timestamp(data);

        event.order_ref = ItchUtil::read_uint64(data + 5);

        event.order_book_id = ItchUtil::read_uint32(data + 13);

        if (!read_side(data[17], event.side)) {
            return false;
        }

        event.shares = ItchUtil::read_uint64(data + 22);

        event.price = ItchUtil::read_int32(data + 30);

        return true;
    }

    bool decode_flush(const unsigned char* data, std::size_t length, Event& event) const noexcept {
        if (length < FLUSH_SIZE) {
            return false;
        }

        event = {};
        event.type = EventType::Flush;

        event.timestamp = timestamp(data);

        event.order_book_id = ItchUtil::read_uint32(data + 5);

        return true;
    }
};

#endif