#ifndef ORDER_BOOK_HPP
#define ORDER_BOOK_HPP

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include "Util.hpp"

struct Order {
    std::uint64_t shares = 0;
    std::int32_t price = 0;
    Side side = Side::Buy;
};

/*
    The resting orders of every order book of the feed.

    Orders are keyed by order book id and order reference: a reference is only
    unique within its book, references are recycled during the day.

    An execute only reduces the shares of an order; it is dropped once it is
    fully executed.
*/

class OrderBook {
public:
    OrderBook() = default;

    void apply(const Event& event);

    void add(const Event& event);
    void execute(const Event& event);
    void remove(std::uint32_t order_book_id, std::uint64_t order_ref);
    void flush(std::uint32_t order_book_id);

    bool best_bid(std::uint32_t order_book_id, Order& order) const;
    bool best_ask(std::uint32_t order_book_id, Order& order) const;

    std::size_t size() const;
    std::size_t book_count() const;

private:
    using Orders = std::unordered_map<std::uint64_t, Order>;

    std::unordered_map<std::uint32_t, Orders> books_;

    bool best(std::uint32_t order_book_id, Side side, Order& best_order) const;

    static bool better(std::int32_t price, std::int32_t best_price, Side side);
};

#endif
