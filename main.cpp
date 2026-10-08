#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include "ItchDecoder.hpp"
#include "OrderBook.hpp"

/*
    The file is a sequence of MoldUDP64 packets.

    session id  10 bytes
    sequence     8 bytes
    count        2 bytes
    then count messages, each: length (2 bytes) + message

    The longest message is 130 bytes (Order Book Directory).
*/

static void print_top_of_book(const OrderBook& book, std::uint32_t order_book_id, std::uint64_t messages) {
    Order bid;
    Order ask;

    std::cout << messages << " msgs";

    if (book.best_bid(order_book_id, bid)) {
        std::cout << "  bid " << bid.price << " x " << bid.shares;
    }

    if (book.best_ask(order_book_id, ask)) {
        std::cout << "  ask " << ask.price << " x " << ask.shares;
    }

    std::cout << '\n';
}

int main(int argc, char** argv) {
    const std::string filename = argc > 1 ? argv[1] : "itch_data_250815_HI2.dat";

    std::ifstream file(filename, std::ios::binary);

    if (!file) {
        std::cerr << "Cannot open: " << filename << '\n';
        return 1;
    }

    ItchDecoder decoder;
    OrderBook book;

    unsigned char header[20];
    unsigned char length_bytes[2];
    unsigned char message[256];

    std::uint64_t messages = 0;
    std::uint64_t events = 0;
    std::uint32_t book_id = 0;
    bool have_book = false;

    while (file.read(reinterpret_cast<char*>(header), sizeof(header))) {
        const std::uint16_t count = ItchUtil::read_uint16(header + 18);

        for (std::uint16_t i = 0; i < count; ++i) {
            if (!file.read(reinterpret_cast<char*>(length_bytes), sizeof(length_bytes))) {
                break;
            }

            const std::uint16_t length = ItchUtil::read_uint16(length_bytes);

            if (!file.read(reinterpret_cast<char*>(message), length)) {
                break;
            }

            ++messages;

            if (have_book && messages % 2000000 == 0) {
                print_top_of_book(book, book_id, messages);
            }

            Event event;

            if (!decoder.decode(message, length, event)) {
                continue;
            }

            if (event.type == EventType::Add && !have_book) {
                book_id = event.order_book_id;
                have_book = true;
            }

            book.apply(event);

            ++events;
        }
    }

    std::cout << "messages: " << messages << '\n';
    std::cout << "events:   " << events << '\n';
    std::cout << "orders:   " << book.size() << '\n';

    return 0;
}
