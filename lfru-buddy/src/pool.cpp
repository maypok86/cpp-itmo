
#include "allocator.h"

#include <pool.h>

namespace {
std::size_t block_size(const std::size_t pool_size)
{
    std::size_t block_size = 0;
    for (std::size_t value = 1; value < pool_size; value <<= 1) {
        ++block_size;
    }
    return block_size;
}

std::size_t pool_size(std::size_t block_size)
{
    return (1U << block_size);
}

} // anonymous namespace

PoolAllocator::PoolAllocator(const unsigned int min_p, const unsigned int max_p)
    : min_block_size(min_p)
    , max_block_size(max_p)
    , m_storage(pool_size(max_p))
    , m_free(max_p - min_p + 1, std::set<std::size_t>())
{
    m_free[to_free_index(max_p)].insert(0);
}

void * PoolAllocator::allocate(const std::size_t n)
{
    const std::size_t min_size = std::max(min_block_size, block_size(n));
    for (std::size_t size = min_size; size <= max_block_size; ++size) {
        if (!m_free[to_free_index(size)].empty()) {
            std::size_t index = delete_index(size);
            for (std::size_t i = min_size; i < size; ++i) {
                const std::size_t current = size - i - 1 + min_size;
                m_free[to_free_index(current)].insert(index);
                m_free[to_free_index(current)].insert(index + pool_size(current));
                index = delete_index(current);
            }
            m_sizes[index] = min_size;
            return &m_storage[index];
        }
    }
    throw std::bad_alloc{};
}

void PoolAllocator::deallocate(const void * ptr)
{
    const std::byte * b_ptr = static_cast<const std::byte *>(ptr);
    if (b_ptr < &m_storage[0] || b_ptr > &m_storage[m_storage.size() - 1]) {
        throw std::bad_alloc{};
    }
    const std::size_t index = b_ptr - &m_storage[0];
    const std::size_t size = m_sizes[index];
    auto current_it = m_free[to_free_index(size)].insert(index).first;
    for (std::size_t current_size = size; current_size < max_block_size; ++current_size) {
        const std::size_t free_index = to_free_index(current_size);
        const std::size_t current = *current_it;
        const std::size_t buddy_number = current / pool_size(current_size);
        const std::size_t buddy_address = buddy_number % 2 == 0 ? current + pool_size(current_size) : current - pool_size(current_size);
        const auto buddy_it = m_free[free_index].find(buddy_address);
        if (buddy_it == m_free[free_index].end()) {
            break;
        }
        m_free[free_index].erase(current_it);
        m_free[free_index].erase(buddy_it);
        if (buddy_number % 2 == 0) {
            current_it = m_free[free_index + 1].insert(current).first;
        }
        else {
            current_it = m_free[free_index + 1].insert(buddy_address).first;
        }
    }
    m_sizes.erase(index);
}
