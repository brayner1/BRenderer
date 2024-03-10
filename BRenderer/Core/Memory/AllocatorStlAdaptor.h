#ifndef BRR_ALLOCATORSTLADAPTOR_H
#define BRR_ALLOCATORSTLADAPTOR_H

namespace brr
{

    template<typename T, typename Alloc>
    class AllocStlAdaptor
    {
    public:

        AllocStlAdaptor() = delete;

        AllocStlAdaptor(Alloc& allocator) noexcept
        : m_allocator(allocator)
        {}

        template<typename U>
        AllocStlAdaptor(const AllocStlAdaptor<U, Alloc>& other) noexcept
        : m_allocator(other.m_allocator)
        {}

        [[nodiscard]] constexpr T* allocate(std::size_t n)
        {
            return reinterpret_cast<T*>
                (m_allocator.allocate(n * sizeof(T), alignof(T)));
        }

        constexpr void deallocate(T* p, [[maybe_unused]] std::size_t n)
        noexcept
        {
            m_allocator.deallocate(p);
        }

        bool operator==(const AllocStlAdaptor<T,Alloc>& rhs) const noexcept
        {
            return *m_allocator == rhs.m_allocator;
        }

        bool operator!=(const AllocStlAdaptor<T,Alloc>& rhs) const noexcept
        {
            return !(*this == rhs);
        }

    private:
        Alloc& m_allocator;
    };

}

#endif