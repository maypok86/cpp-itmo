#pragma once

#include <cstddef>
#include <list>
#include <map>
#include <new>
#include <set>
#include <vector>

class PoolAllocator
{
public:
    PoolAllocator(unsigned min_p, unsigned max_p);

    void * allocate(std::size_t n);

    void deallocate(const void * ptr);

private:
    const std::size_t min_block_size;
    const std::size_t max_block_size;
    std::vector<std::byte> m_storage;
    std::vector<std::set<std::size_t>> m_free;
    std::map<std::size_t, std::size_t> m_sizes;

    std::size_t delete_index(const std::size_t size)
    {
        const std::size_t index = *m_free[to_free_index(size)].begin();
        // amortized O(1)
        // if call erase(index) then O(logn)
        m_free[to_free_index(size)].erase(m_free[to_free_index(size)].begin());
        return index;
    }

    std::size_t to_free_index(const std::size_t index)
    {
        return index - min_block_size;
    }
};
