#ifndef BRR_RESOURCECONTAINER_H
#define BRR_RESOURCECONTAINER_H

namespace brr
{
    class BlockAllocator
    {
    public:

        BlockAllocator(uint32_t block_size = 250 * 10e6);

        template <typename T, typename... Args>
        T* AllocateResource(Args... args);

    private:
        struct MemoryBlock
        {
            void* m_allocated_memory = nullptr;
            uint32_t m_total_size = 0;
            uint32_t m_remaining_size = 0;
        };

        BlockAllocator::MemoryBlock* AllocateNewBlock();
        void* AllocateMemory(uint32_t size);

        std::list<MemoryBlock> m_available_blocks;
        std::list<MemoryBlock> m_full_blocks;
        uint32_t m_default_block_size;
    };

    template <typename T, typename ... Args>
    T* BlockAllocator::AllocateResource(Args... args)
    {
    }
}

#endif