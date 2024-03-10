#ifndef BRR_ARENAMEMORY_H
#define BRR_ARENAMEMORY_H

#include <list>
#include <memory_resource>

namespace brr::mem
{
    class ArenaMemory : public std::pmr::memory_resource
    {
    public:

        ArenaMemory(size_t chunk_size = 262144);

        ArenaMemory(std::pmr::memory_resource* upstream, size_t chunk_size = 262144);

        ArenaMemory(ArenaMemory&& other) noexcept;

        ArenaMemory(const ArenaMemory&) = delete;

        ~ArenaMemory() override;


        /**
         * \brief Make all the arenas memory free. Do not deallocate.
         */
        void Release();

    private:
        
        template<typename T>
        friend class AllocatorStlAdaptor;

        void* do_allocate(size_t bytes, size_t align) override;
        void do_deallocate(void* pointer, size_t bytes, size_t align) override;

        bool do_is_equal(const memory_resource& other) const noexcept override;

        std::pmr::memory_resource* m_upstream;

        size_t m_current_block_pos = 0, m_current_alloc_size = 0;
        uint8_t* m_current_block = nullptr;
        std::list<std::pair<size_t, uint8_t*>> m_used_blocks, m_available_blocks;

        const size_t m_chunk_size;
    };

}

#endif