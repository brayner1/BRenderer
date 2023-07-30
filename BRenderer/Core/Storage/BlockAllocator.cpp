#include "Core/Storage/BlockAllocator.h"

namespace brr
{
    BlockAllocator::MemoryBlock* BlockAllocator::AllocateNewBlock()
    {
        MemoryBlock block;

#ifdef _MSC_VER
        block.m_allocated_memory = _aligned_malloc(m_default_block_size, 16);
#else
        block.m_allocated_memory = std::aligned_alloc(16, m_default_block_size);
#endif

        if (block.m_allocated_memory)
        {
            block.m_remaining_size = block.m_total_size = m_default_block_size;

            m_available_blocks.push_back(block);

            return &m_available_blocks.back();
        }

        return nullptr;
    }

    void* BlockAllocator::AllocateMemory(uint32_t size)
    {
        MemoryBlock* selected_block;
        for (MemoryBlock& block : m_available_blocks)
        {
            if (block.m_remaining_size >= size)
            {
                selected_block = &block;
                break;
            }
        }

        if (!selected_block)
        {
            selected_block = AllocateNewBlock();
        }

        if (!selected_block)
        {
            return nullptr;
        }


    }

}
