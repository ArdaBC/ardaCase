#include "OrderBook.hpp"

void OrderBook::apply(const Event& event) {
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

void OrderBook::add(const Event& event) {
    Order& order = books_[event.order_book_id][event.order_ref];

    order.shares = event.shares;
    order.price = event.price;
    order.side = event.side;
}

void OrderBook::execute(const Event& event) {
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

void OrderBook::remove(std::uint32_t order_book_id, std::uint64_t order_ref) {
    const auto book = books_.find(order_book_id);

    if (book == books_.end()) {
        return;
    }

    book->second.erase(order_ref);
}

void OrderBook::flush(std::uint32_t order_book_id) {
    books_.erase(order_book_id);
}

bool OrderBook::best_bid(std::uint32_t order_book_id, Order& order) const {
    return best(order_book_id, Side::Buy, order);
}

bool OrderBook::best_ask(std::uint32_t order_book_id, Order& order) const {
    return best(order_book_id, Side::Sell, order);
}

std::size_t OrderBook::size() const {
    std::size_t total = 0;

    for (const auto& book : books_) {
        total += book.second.size();
    }

    return total;
}

std::size_t OrderBook::book_count() const {
    return books_.size();
}

bool OrderBook::best(std::uint32_t order_book_id, Side side, Order& best_order) const {
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

bool OrderBook::better(std::int32_t price, std::int32_t best_price, Side side) {
    if (side == Side::Buy) {
        return price > best_price;
    }

    return price < best_price;
}
