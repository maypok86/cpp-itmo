#pragma once

#include "allocator.h"

#include <algorithm>
#include <cstddef>
#include <list>
#include <new>
#include <ostream>

namespace {
template <class KeyProvider>
void print_cache(std::ostream & strm, const std::list<KeyProvider *> & cache)
{
    if (cache.empty()) {
        strm << "<empty>";
    }
    else {
        bool first = true;
        for (const auto x : cache) {
            if (!first) {
                strm << " ";
            }
            else {
                first = false;
            }
            strm << *x;
        }
    }
}

} // anonymous namespace

template <class Key, class KeyProvider, class Allocator>
class Cache
{
public:
    template <class... AllocArgs>
    Cache(const std::size_t cache_size, AllocArgs &&... alloc_args)
        : m_max_top_size(cache_size)
        , m_max_low_size(cache_size)
        , m_alloc(std::forward<AllocArgs>(alloc_args)...)
    {
    }

    std::size_t size() const
    {
        return lru.size() + fifo.size();
    }

    bool empty() const
    {
        return lru.empty() && fifo.empty();
    }

    template <class T>
    T & get(const Key & key);

    std::ostream & print(std::ostream & strm) const;

    friend std::ostream & operator<<(std::ostream & strm, const Cache & cache)
    {
        return cache.print(strm);
    }

private:
    const std::size_t m_max_top_size;
    const std::size_t m_max_low_size;
    std::list<KeyProvider *> lru;
    std::list<KeyProvider *> fifo;
    Allocator m_alloc;
};

template <class Key, class KeyProvider, class Allocator>
template <class T>
T & Cache<Key, KeyProvider, Allocator>::get(const Key & key)
{
    const auto test = [&key](const KeyProvider * elem) {
        return *elem == key;
    };
    const auto lru_it = std::find_if(lru.begin(), lru.end(), test);
    if (lru_it != lru.end()) {
        lru.splice(lru.begin(), lru, lru_it);
    }
    else {
        const auto fifo_it = std::find_if(fifo.begin(), fifo.end(), test);
        if (fifo_it == fifo.end()) {
            KeyProvider * new_elem = m_alloc.template create<T>(key);
            if (m_max_low_size == fifo.size()) {
                m_alloc.template destroy<KeyProvider>(fifo.back());
                fifo.pop_back();
            }
            fifo.push_front(new_elem);
            return *static_cast<T *>(new_elem);
        }
        if (m_max_top_size == lru.size()) {
            fifo.push_front(lru.back());
            lru.pop_back();
        }
        lru.splice(lru.begin(), fifo, fifo_it);
    }
    return *static_cast<T *>(lru.front());
}

template <class Key, class KeyProvider, class Allocator>
std::ostream & Cache<Key, KeyProvider, Allocator>::print(std::ostream & strm) const
{
    strm << "Priority: ";
    print_cache(strm, lru);
    strm << "\nRegular: ";
    print_cache(strm, fifo);
    strm << "\n";
    return strm;
}
