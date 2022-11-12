#pragma once

#include <iterator>
#include <iostream>
#include <vector>

template <typename It>
class IteratorPair {
    public:
        explicit IteratorPair(It begin, It end);
        It begin() const;
        It end() const;
        int size() const;
    private:
        It begin_;
        It end_;
};

template <typename It>
class Paginator {
    public:
        Paginator(It begin, It end, size_t page_size);
        auto begin() const;
        auto end() const;
    private:
       std::vector<IteratorPair<It>> pages_;
};

template <typename It>
IteratorPair<It>::IteratorPair(It begin, It end) 
            : begin_(begin)
            , end_(end) {
        }
template <typename It>
It IteratorPair<It>::begin() const {
    return begin_;
}
template <typename It>
It IteratorPair<It>::end() const {
    return end_;
}
template <typename It>
int IteratorPair<It>::size() const {
    return std::distance(begin_, end_);
}

template <typename It>
Paginator<It>::Paginator(It begin, It end, size_t page_size) {
    const size_t full_pages_count = std::distance(begin, end) / page_size;
    for (size_t i = 0; i < full_pages_count; ++i) {
        pages_.push_back(IteratorPair<It>(begin, std::next(begin, page_size)));
        std::advance(begin, page_size);
    }
    if (std::distance(begin, end) > 0) pages_.push_back(IteratorPair<It>(begin, end));
}
template <typename It>
auto Paginator<It>::begin() const {
    return pages_.begin();
}
template <typename It>
auto Paginator<It>::end() const {
    return pages_.end();
}

template <typename It>
std::ostream& operator<<(std::ostream& os, const IteratorPair<It> page) {
    for (auto it = page.begin(); it != page.end(); std::advance(it, 1)) {
        os << *it;
    }
    return os;
}

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}