#ifndef BRR_LINEARMEMORY_H
#define BRR_LINEARMEMORY_H

#include <memory_resource>

namespace brr::mem
{
    class LinearMemory : public std::pmr::memory_resource
    {
    public:

        LinearMemory(size_t memory_size, std::pmr::memory_resource* upstream_resource);

        ~LinearMemory() override;

        constexpr size_t UsedSpace() const { return m_current_pos; }
        constexpr size_t MaxSpace() const { return m_mem_size; }
        constexpr size_t RemainingSpace() const { return m_mem_size - m_current_pos; }

    private:

        template<typename T>
        friend class AllocatorStlAdaptor;

        void* do_allocate(size_t bytes, size_t align) override;
        void do_deallocate(void* ptr, size_t bytes, size_t align) override;

        bool do_is_equal(const memory_resource& other) const noexcept override;

        std::pmr::memory_resource* m_upstream;

        size_t m_mem_size;
        void* m_mem_block;
        size_t m_current_pos;
    };
}

#endif