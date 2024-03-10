#ifndef BRR_STACKMEMORY_H
#define BRR_STACKMEMORY_H

#include <memory_resource>

namespace brr::mem
{
    class StackMemory : public std::pmr::memory_resource
    {
    public:

        StackMemory(size_t memory_size, std::pmr::memory_resource* upstream);

    private:

        template<typename T>
        friend class AllocatorStlAdaptor;

        void* do_allocate(size_t bytes, size_t align) override;
        void do_deallocate(void* ptr, size_t bytes, size_t align) override;

        bool do_is_equal(const memory_resource& other) const noexcept override;

        size_t m_mem_size;
        void*  m_mem_block;

        std::pmr::memory_resource* m_upstream;
    };
}

#endif