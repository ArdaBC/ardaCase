#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <getopt.h>
#include "ItchDecoder.hpp"
#include "MoldUdp64.hpp"
#include "OrderBook.hpp"

/*
    Decodes a BISTECH ITCH feed into order books.

    The feed is a stream of MoldUDP64 packets read from a UDP socket
    (multicast or unicast) or from a capture file.
*/

static const std::uint16_t DEFAULT_PORT = 12140;
static const std::uint64_t DEFAULT_SNAPSHOT = 2000000;

static void print_usage(const char* program) {
    std::cout
        << "usage: " << program << " [options]\n"
        << '\n'
        << "  -f, --file <path>       read MoldUDP64 packets from a capture file\n"
        << "  -p, --port <port>       UDP port to receive on (default " << DEFAULT_PORT << ")\n"
        << "  -g, --group <address>   multicast group to join (default: unicast)\n"
        << "  -i, --interface <addr>  local interface used for the multicast join\n"
        << "  -s, --snapshot <n>      print top of book every n messages (default " << DEFAULT_SNAPSHOT << ")\n"
        << "  -l, --limit <n>         stop after n messages (default: no limit)\n"
        << "  -h, --help\n";
}

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

    // A live feed prints for a long time before the buffer would flush, so the
    // periodic lines and the banner are flushed explicitly.
    std::cout << std::endl;
}

int main(int argc, char** argv) {
    std::string filename;
    std::string group;
    std::string interface_address;
    std::uint16_t port = DEFAULT_PORT;
    std::uint64_t snapshot = DEFAULT_SNAPSHOT;
    std::uint64_t limit = 0;

    static const option options[] = {
        {"file", required_argument, nullptr, 'f'},
        {"port", required_argument, nullptr, 'p'},
        {"group", required_argument, nullptr, 'g'},
        {"interface", required_argument, nullptr, 'i'},
        {"snapshot", required_argument, nullptr, 's'},
        {"limit", required_argument, nullptr, 'l'},
        {"help", no_argument, nullptr, 'h'},
        {nullptr, 0, nullptr, 0}
    };

    int option_code = 0;

    while ((option_code = getopt_long(argc, argv, "f:p:g:i:s:l:h", options, nullptr)) != -1) {
        switch (option_code) {
            case 'f':
                filename = optarg;
                break;

            case 'p':
                port = static_cast<std::uint16_t>(std::strtoul(optarg, nullptr, 10));

                if (port == 0) {
                    std::cerr << "Bad port: " << optarg << '\n';
                    return 1;
                }

                break;

            case 'g':
                group = optarg;
                break;

            case 'i':
                interface_address = optarg;
                break;

            case 's':
                snapshot = std::strtoull(optarg, nullptr, 10);
                break;

            case 'l':
                limit = std::strtoull(optarg, nullptr, 10);
                break;

            case 'h':
                print_usage(argv[0]);
                return 0;

            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    MoldUdp64Connector connector;

    if (!filename.empty()) {
        if (!connector.open_file(filename)) {
            return 1;
        }
    } else {
        if (!connector.open_socket(port, group, interface_address)) {
            return 1;
        }

        std::cout << "listening on udp " << (group.empty() ? "0.0.0.0" : group) << ':' << port << std::endl;
    }

    ItchDecoder decoder;
    OrderBook book;

    std::uint64_t events = 0;
    std::uint32_t book_id = 0;
    bool have_book = false;

    const unsigned char* message = nullptr;
    std::size_t length = 0;

    while (connector.next_message(message, length)) {
        const std::uint64_t messages = connector.messages();

        // The snapshot is taken before decoding, so that a milestone is not
        // skipped when that message does not decode into an event.
        if (have_book && snapshot > 0 && messages % snapshot == 0) {
            print_top_of_book(book, book_id, messages);
        }

        if (limit > 0 && messages >= limit) {
            break;
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

    std::cout << "messages: " << connector.messages() << '\n';
    std::cout << "events:   " << events << '\n';
    std::cout << "orders:   " << book.size() << '\n';
    std::cout << "session:  " << connector.session() << '\n';
    std::cout << "packets:  " << connector.packets() << '\n';

    if (connector.gaps() > 0) {
        std::cout << "gaps:     " << connector.gaps() << " (" << connector.missed() << " messages missed)\n";
    }

    return 0;
}
