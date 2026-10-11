## Build

    make

## Run

    ./main -f capture.dat                        replay a capture file
    ./main -p 12140                              receive unicast on a port
    ./main -p 12140 -g 239.1.2.3 -i 10.0.0.5     join a multicast group

Options:

    -f, --file <path>       read MoldUDP64 packets from a capture file
    -p, --port <port>       UDP port to receive on (default 12140)
    -g, --group <address>   multicast group to join (default: unicast)
    -i, --interface <addr>  local interface used for the multicast join
    -s, --snapshot <n>      print top of book every n messages (default 2000000)
    -l, --limit <n>         stop after n messages (default: no limit)
    -h, --help

## Output

A snapshot line, printed every `-s` messages:

    3000000 msgs  bid 7470 x 3744  ask 7480 x 7000
    (best bid at 7,470 with 3,744 shares wanted and best ask at 7,480 with 7,000 shares offered)

and a summary when the feed ends:

    messages: 15560041
    events:   15423310
    orders:   0
    session:  TR29253481
    packets:  519492
    gaps:     1 (54 messages missed)

`messages` counts every message handed over by the transport, `events` the ones
that became an event.

## Files

    Util.hpp/.cpp           big-endian field reads, side and timestamp helpers,
                            Event / Side / EventType
    MoldUdp64.hpp/.cpp      transport: packets, session and sequence tracking
    ItchDecoder.hpp/.cpp    turns one message into one Event
    OrderBook.hpp/.cpp      the resting orders of every order book
    main.cpp                command line, decode loop, output

## How it works

1. The connector reads one MoldUDP64 packet and hands out its messages one by
   one as `(pointer, length)`. It knows nothing about the messages themselves.
2. The decoder turns a message into an Event (add, execute, delete, replace,
   flush) and rejects anything else, for example the directory or tick size
   messages.
3. The order book applies the event. Orders are keyed by order book id and order
   reference: a reference is only unique inside its book and gets recycled
   during the day.
