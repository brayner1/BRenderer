#include "ArenaMemory.h"

#include "Core/LogSystem.h"

using namespace brr::mem;

ArenaMemory::ArenaMemory(size_t chunk_size)
: m_upstream(std::pmr::get_default_resource()),
  m_chunk_size(chunk_size)
{}

ArenaMemory::ArenaMemory(std::pmr::memory_resource* upstream,
                         size_t                     chunk_size)
: m_upstream(upstream),
  m_chunk_size(chunk_size)
{}

ArenaMemory::ArenaMemory(ArenaMemory&& other) noexcept
: m_upstream(other.m_upstream),
  m_chunk_size(other.m_chunk_size)
{
    std::swap(m_current_block_pos, other.m_current_block_pos);
    std::swap(m_current_alloc_size, other.m_current_alloc_size);
    std::swap(m_current_block, other.m_current_block);
    std::swap(m_used_blocks, other.m_used_blocks);
    std::swap(m_available_blocks, other.m_available_blocks);
}

ArenaMemory::~ArenaMemory()
{
    for (auto& block: m_available_blocks)
    {
        m_upstream->deallocate((void*)block.second, block.first);
    }

    for (auto& block : m_used_blocks)
    {
        m_upstream->deallocate((void*)block.second, block.first);
    }
}

void ArenaMemory::Release()
{
    m_available_blocks.emplace_back (m_current_alloc_size, m_current_block);
    m_current_block = nullptr;
    m_current_alloc_size = 0;

    for (const auto& [size, pntr] : m_used_blocks)
    {
        m_available_blocks.emplace_back (size, pntr);
    }
    m_used_blocks.clear();
}

void* ArenaMemory::do_allocate(size_t bytes_size,
                               size_t alignment)
{
    size_t bytes_size_algn = (bytes_size + alignment - 1) & ~(alignment - 1);
    if (m_current_block_pos + bytes_size_algn > m_current_alloc_size)
    {
        if (m_current_block)
        {
            m_used_blocks.emplace_back (m_current_alloc_size, m_current_block);
            m_current_block = nullptr;
            m_current_alloc_size = 0;
        }

        for (auto block_iter = m_available_blocks.begin();
             block_iter != m_available_blocks.end();
             block_iter++)
        {
            if (block_iter->first >= bytes_size_algn)
            {
                m_current_block = block_iter->second;
                m_current_alloc_size = block_iter->first;
            }
        }

        if (!m_current_block)
        {
            // If did not found available blocks, add block
            m_current_alloc_size = std::max(m_chunk_size, bytes_size_algn);
            m_current_block = static_cast<uint8_t*>(m_upstream->allocate(m_current_alloc_size));
        }

        m_current_block_pos = 0;
    }

    if (!m_current_block)
    {
        // Out of Memory!
        BRR_LogError("Out of Memory on ArenaAllocator ({})", (uint64_t)this);
        return nullptr;
    }

    void* ret = m_current_block + m_current_block_pos;
    void* current_ptr = m_current_block + m_current_block_pos;
    size_t remaining_space = m_current_alloc_size - m_current_block_pos;
    if (std::align(alignment, bytes_size, current_ptr, remaining_space))
    {
        m_current_block_pos += (m_current_alloc_size - remaining_space) + bytes_size;
        return current_ptr;
    }
    return nullptr;
}

void ArenaMemory::do_deallocate(void* pointer,
                                size_t bytes,
                                size_t alignment)
{
    (void)pointer;
    (void)bytes;
    (void)alignment;
}

bool ArenaMemory::do_is_equal(const memory_resource& other) const noexcept
{
    const ArenaMemory* arena_memory = dynamic_cast<const ArenaMemory*>(&other);
    if (arena_memory == nullptr) return false;

    return this == arena_memory;
}