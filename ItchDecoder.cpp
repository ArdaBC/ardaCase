#include "ItchDecoder.hpp"

bool ItchDecoder::decode(const unsigned char* data, std::size_t length, Event& event) noexcept {
    if (data == nullptr || length == 0) {
        return false;
    }

    const char type = static_cast<char>(data[0]);

    switch (type) {
        case 'T':
            return decode_timestamp(data, length);

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

void ItchDecoder::reset() noexcept {
    timestamp_seconds_ = 0;
}

bool ItchDecoder::decode_timestamp(const unsigned char* data, std::size_t length) noexcept {
    if (length < TIMESTAMP_SIZE) {
        return false;
    }

    timestamp_seconds_ = Util::read_uint32(data + 1);

    return false; // No Event generated
}

void ItchDecoder::set_prefix(const OrderPrefix& prefix, EventType type, Event& event) const noexcept {
    event = {};
    event.type = type;
    event.timestamp = prefix.timestamp;
    event.order_ref = prefix.order_ref;
    event.order_book_id = prefix.order_book_id;
    event.side = prefix.side;
}

bool ItchDecoder::decode_add(const unsigned char* data, std::size_t length, Event& event) const noexcept {
    if (length < ADD_SIZE) {
        return false;
    }

    OrderPrefix prefix;

    if (!Util::read_order_prefix(data, timestamp_seconds_, prefix)) {
        return false;
    }

    set_prefix(prefix, EventType::Add, event);

    event.shares = Util::read_uint64(data + 22);
    event.price = Util::read_int32(data + 30);

    return true;
}

bool ItchDecoder::decode_execute(const unsigned char* data, std::size_t length, Event& event) const noexcept {
    if (length < EXEC_SIZE) {
        return false;
    }

    OrderPrefix prefix;

    if (!Util::read_order_prefix(data, timestamp_seconds_, prefix)) {
        return false;
    }

    set_prefix(prefix, EventType::Execute, event);

    event.shares = Util::read_uint64(data + 18);

    return true;
}

bool ItchDecoder::decode_execute_with_price(const unsigned char* data, std::size_t length, Event& event) const noexcept {
    if (length < EXEC_P_SIZE) {
        return false;
    }

    OrderPrefix prefix;

    if (!Util::read_order_prefix(data, timestamp_seconds_, prefix)) {
        return false;
    }

    set_prefix(prefix, EventType::ExecuteWithPrice, event);

    event.shares = Util::read_uint64(data + 18);
    event.price = Util::read_int32(data + 52);

    return true;
}

bool ItchDecoder::decode_delete(const unsigned char* data, std::size_t length, Event& event) const noexcept {
    if (length < DELETE_SIZE) {
        return false;
    }

    OrderPrefix prefix;

    if (!Util::read_order_prefix(data, timestamp_seconds_, prefix)) {
        return false;
    }

    set_prefix(prefix, EventType::Delete, event);

    return true;
}

bool ItchDecoder::decode_replace(const unsigned char* data, std::size_t length, Event& event) const noexcept {
    if (length < REPLACE_SIZE) {
        return false;
    }

    OrderPrefix prefix;

    if (!Util::read_order_prefix(data, timestamp_seconds_, prefix)) {
        return false;
    }

    set_prefix(prefix, EventType::Replace, event);

    event.shares = Util::read_uint64(data + 22);
    event.price = Util::read_int32(data + 30);

    return true;
}

bool ItchDecoder::decode_flush(const unsigned char* data, std::size_t length, Event& event) const noexcept {
    if (length < FLUSH_SIZE) {
        return false;
    }

    event = {};
    event.type = EventType::Flush;
    event.timestamp = Util::make_timestamp(timestamp_seconds_, Util::read_uint32(data + 1));
    event.order_book_id = Util::read_uint32(data + 5);

    return true;
}
