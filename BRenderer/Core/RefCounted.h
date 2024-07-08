#ifndef BRR_REFCOUNTED_H
#define BRR_REFCOUNTED_H

#include <atomic>
#include <concepts>

namespace brr
{
    class RefCounted;

    template <typename Ty>
    concept RefCountedDerivedT = std::derived_from<Ty, RefCounted>;

    class RefCounted
    {
    public:
        void Reference()
        {
            m_ref_count.fetch_add(1, std::memory_order_acq_rel);
        }

        bool Dereference()
        {
            return (m_ref_count.fetch_sub(1, std::memory_order_acq_rel) - 1) == 0;
        }

        bool IsReferenced() const { return m_ref_count.load(std::memory_order_acquire) > 0; }

        uint32_t RefCount() const { return m_ref_count.load(std::memory_order_acquire); }

    private:
        template <RefCountedDerivedT T>
        friend class Ref;

        std::atomic<uint32_t> m_ref_count;
    };
}

#endif