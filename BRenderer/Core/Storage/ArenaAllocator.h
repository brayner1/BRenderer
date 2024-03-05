#ifndef BRR_ARENAALLOCATOR_H
#define BRR_ARENAALLOCATOR_H

#include <list>
#include <memory_resource>

namespace brr
{

  class ArenaMemory : public std::pmr::memory_resource
  {
  public:

    ArenaMemory(size_t chunk_size = 262144);

    ArenaMemory(std::pmr::memory_resource* upstream, size_t chunk_size = 262144);

  private:

    void* do_allocate(size_t _Bytes, size_t _Align) override;
    void do_deallocate(void* _Ptr, size_t _Bytes, size_t _Align) override;
    bool do_is_equal(const memory_resource& _That) const noexcept override;

    void AddBlock();

    std::pmr::memory_resource* m_upstream;

    size_t m_current_block_pos = 0, currentAllocSize = 0;
    uint8_t* m_current_block = nullptr;
    std::list<std::pair<size_t, uint8_t*>> m_used_blocks, m_available_blocks;

    const size_t m_chunk_size;
  };

}

#endif