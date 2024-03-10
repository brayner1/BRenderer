#include "LinearMemory.h"

using namespace brr::mem;

LinearMemory::LinearMemory(size_t memory_size, std::pmr::memory_resource* upstream_resource)
: m_upstream(upstream_resource),
  m_mem_size(memory_size)
{
    m_mem_block = m_upstream->allocate(m_mem_size);
    m_current_pos = 0;
}

LinearMemory::~LinearMemory()
{
    m_upstream->deallocate(m_mem_block, m_mem_size);
}

void* LinearMemory::do_allocate(size_t bytes, size_t align)
{
    size_t remaining_space = m_mem_size - m_current_pos;
    if (bytes > remaining_space)
        return nullptr;

    void* current_ptr = static_cast<uint8_t*>(m_mem_block) + m_current_pos;
    if (std::align(align, bytes, current_ptr, remaining_space))
    {
        m_current_pos = (m_mem_size - remaining_space) + bytes;
        return current_ptr;
    }
    return nullptr;
}

void LinearMemory::do_deallocate(void* ptr, size_t bytes, size_t align)
{
}

bool LinearMemory::do_is_equal(const memory_resource& other) const noexcept
{
    const LinearMemory* linear_memory = dynamic_cast<const LinearMemory*>(&other);
    if (linear_memory == nullptr) return false;

    return this == linear_memory;
}
