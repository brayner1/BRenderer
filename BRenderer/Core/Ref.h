#ifndef BRR_REF_H
#define BRR_REF_H

#include <Core/RefCounted.h>

#include <concepts>

namespace brr
{
    template <RefCountedDerivedT Ty>
    class Ref
    {
    public:

        Ref()
        : m_ref(nullptr)
        {}

        explicit Ref(Ty* reference)
        {
            Reference(reference);
        }

        Ref(const Ref<Ty>& other)
        {
            *this = other;
        }

        Ref(Ref<Ty>&& other) noexcept
        {
            *this = std::move(other);
        }

        template <class T_Other>
	    Ref(const Ref<T_Other> &p_from) {
		    Ty* converted_ptr = dynamic_cast<Ty*>(p_from.Ptr());
            if (!converted_ptr)
            {
                Unreference();
                return;
            }

            Reference(converted_ptr);
	    }

        ~Ref()
        {
            Unreference();
        }

        Ref<Ty>& operator=(Ty* reference)
        {
            Reference(reference);
            return *this;
        }

        Ref<Ty>& operator=(const Ref<Ty>& other)
        {
            *this = other.m_ref;
            return *this;
        }

        Ref<Ty>& operator=(Ref<Ty>&& other)
        {
            m_ref = other.m_ref;
            other.m_ref = nullptr;
        }

        Ty& operator*() const { return *m_ref; }

        Ty* operator->() const { return m_ref; }

        Ty* Ptr() const { return m_ref; }

    private:

        void Reference(Ty* reference)
        {
            if (m_ref == reference)
            {
                return;
            }

            Unreference();

            m_ref = reference;
            if (m_ref)
            {
                m_ref->Reference();
            }
        }

        void Unreference()
        {
            if (m_ref && m_ref->Dereference())
            {
                delete m_ref;
            }

            m_ref = nullptr;
        }

        Ty* m_ref;
    };
}

#endif