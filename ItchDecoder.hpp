#ifndef ITCH_DECODER_HPP
#define ITCH_DECODER_HPP

#include <cstddef>
#include <cstdint>
#include "Util.hpp"

/*
    Turns BISTECH ITCH messages into events.

    The decoder is stateless apart from the current second, which the
    timestamp message ('T') sets and the order messages carry as an offset
    inside that second. It ignores every message it cannot turn into an
    event (directory, tick size, trade, ...).

    "T" Timestamp Seconds          5 bytes   (sets the current second)
    "A" Add Order                 45 bytes
    "E" Order Executed            52 bytes
    "C" Order Executed with Price 58 bytes
    "U" Order Replace             36 bytes   (not used by BIST)
    "D" Order Delete              18 bytes
    "Y" Order Book Flush           9 bytes

    Message sizes are the ITCH sizes; a shorter buffer is rejected rather
    than read past its end.
*/

class ItchDecoder {
public:
    ItchDecoder() = default;

    // Decodes one message. Returns false when the message is malformed or
    // does not produce an event.
    bool decode(const unsigned char* data, std::size_t length, Event& event) noexcept;

    // Forgets the current second.
    void reset() noexcept;

private:
    static constexpr std::size_t TIMESTAMP_SIZE = 5;
    static constexpr std::size_t ADD_SIZE       = 45;
    static constexpr std::size_t EXEC_SIZE      = 52;
    static constexpr std::size_t EXEC_P_SIZE    = 58;
    static constexpr std::size_t DELETE_SIZE    = 18;
    static constexpr std::size_t REPLACE_SIZE   = 36;
    static constexpr std::size_t FLUSH_SIZE     = 9;

    std::uint32_t timestamp_seconds_ = 0;

    bool decode_timestamp(const unsigned char* data, std::size_t length) noexcept;

    bool decode_add(const unsigned char* data, std::size_t length, Event& event) const noexcept;
    bool decode_execute(const unsigned char* data, std::size_t length, Event& event) const noexcept;
    bool decode_execute_with_price(const unsigned char* data, std::size_t length, Event& event) const noexcept;
    bool decode_delete(const unsigned char* data, std::size_t length, Event& event) const noexcept;
    bool decode_replace(const unsigned char* data, std::size_t length, Event& event) const noexcept;
    bool decode_flush(const unsigned char* data, std::size_t length, Event& event) const noexcept;

    // Copies the prefix of an order message into the event.
    void set_prefix(const OrderPrefix& prefix, EventType type, Event& event) const noexcept;
};

#endif
