#ifndef ORDER_BOOK_HPP
#define ORDER_BOOK_HPP

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include "ItchUtil.hpp"

struct Order {
    std::uint64_t shares = 0;
    std::int32_t price = 0;
    Side side = Side::Buy;
};

class OrderBook {
public:
    OrderBook() = default;

    void apply(const Event& event) {
        switch (event.type) {
            case EventType::Add:
            case EventType::Replace:
                add(event);
                break;

            case EventType::Execute:
            case EventType::ExecuteWithPrice:
                execute(event);
                break;

            case EventType::Delete:
                remove(event.order_book_id, event.order_ref);
                break;

            case EventType::Flush:
                flush(event.order_book_id);
                break;

            default:
                break;
        }
    }

    void add(const Event& event) {
        Order& order = books_[event.order_book_id][event.order_ref];

        order.shares = event.shares;
        order.price = event.price;
        order.side = event.side;
    }

    void execute(const Event& event) {
        const auto book = books_.find(event.order_book_id);

        if (book == books_.end()) {
            return;
        }

        const auto order = book->second.find(event.order_ref);

        if (order == book->second.end()) {
            return;
        }

        if (event.shares >= order->second.shares) {
            book->second.erase(order);
            return;
        }

        order->second.shares -= event.shares;
    }

    void remove(std::uint32_t order_book_id, std::uint64_t order_ref) {
        const auto book = books_.find(order_book_id);

        if (book == books_.end()) {
            return;
        }

        book->second.erase(order_ref);
    }

    void flush(std::uint32_t order_book_id) {
        books_.erase(order_book_id);
    }

    bool best_bid(std::uint32_t order_book_id, Order& order) const {
        return best(order_book_id, Side::Buy, order);
    }

    bool best_ask(std::uint32_t order_book_id, Order& order) const {
        return best(order_book_id, Side::Sell, order);
    }

    std::size_t size() const {
        std::size_t total = 0;

        for (const auto& book : books_) {
            total += book.second.size();
        }

        return total;
    }

    std::size_t book_count() const {
        return books_.size();
    }

private:
    std::unordered_map<std::uint32_t, std::unordered_map<std::uint64_t, Order>> books_;

    bool best(std::uint32_t order_book_id, Side side, Order& best_order) const {
        const auto book = books_.find(order_book_id);

        if (book == books_.end()) {
            return false;
        }

        bool found = false;

        for (const auto& entry : book->second) {
            const Order& order = entry.second;

            if (order.side != side) {
                continue;
            }

            if (!found || better(order.price, best_order.price, side)) {
                best_order = order;
                found = true;
            }
        }

        return found;
    }

    static bool better(std::int32_t price, std::int32_t best_price, Side side) {
        if (side == Side::Buy) {
            return price > best_price;
        }

        return price < best_price;
    }
};

#endif
