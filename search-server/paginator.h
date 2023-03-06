#pragma once

#include <vector>

template <typename Iterator>
class Paginator {
    public:
        Paginator(Iterator container_begin, Iterator container_end, const size_t page_size) {
            while(container_begin < container_end) {
                if (static_cast <size_t> (distance(container_begin, container_end)) < page_size) {
                    pages_.push_back({container_begin, container_begin});
                } else {
                    pages_.push_back({container_begin,  container_begin + page_size - 1});
                }
                advance(container_begin, page_size);
            }
        }

        size_t size() const{
            return pages_.size();
        }

        auto begin() const {
            return pages_.begin();
        }

        auto end() const {
            return pages_.end();
        }

    private:
        std::vector <std::pair <Iterator, Iterator>> pages_;
};

template <typename Container>
auto Paginate(const Container& container, size_t page_size) {
    return Paginator(container.begin(), container.end(), page_size);
}
