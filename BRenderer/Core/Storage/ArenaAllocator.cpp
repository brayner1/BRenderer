#include "ArenaAllocator.h"

using namespace brr;

ArenaMemory::ArenaMemory(size_t chunk_size)
: m_upstream(std::pmr::get_default_resource()),
  m_chunk_size(chunk_size)
{}

ArenaMemory::ArenaMemory(std::pmr::memory_resource* upstream,
                         size_t                     chunk_size)
: m_upstream(upstream),
  m_chunk_size(chunk_size)
{}

void* ArenaMemory::do_allocate(size_t _Bytes,
                               size_t _Align)
{
    if (m_current_block == nullptr)
    {
        
    }
}

void ArenaMemory::do_deallocate(void* _Ptr,
    size_t _Bytes,
    size_t _Align)
{
    
}

bool ArenaMemory::do_is_equal(const memory_resource& _That) const noexcept
{
    
}

void ArenaMemory::AddBlock()
{
    
}
