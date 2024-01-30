#ifndef BRR_DYNAMICPOOL_H
#define BRR_DYNAMICPOOL_H
#include <cassert>
#include <cstdint>
#include <vector>
#include <unordered_map>

namespace brr
{
    /**
     * \brief Templated dynamic pool that stores all its objects in contiguous memory.
     *        Since all active objects are stored in contiguous memory, iterating through the pool is very fast.
     *        Creating a new object generates a new ObjectId, which is unique per pool instance.
     *        Accessing an individual object involves an indirect memory access
     *        using the ObjectId identifier to obtain the object index.
     *        That means accessing elements this way is not the fastest way.
     */
    template<typename T>
    class ContiguousPool
    {
    public:
        using ObjectId = uint32_t;
        using iterator = typename std::vector<T>::iterator;
        using const_iterator = typename std::vector<T>::const_iterator;

        explicit ContiguousPool(uint32_t initial_size = 32);

        ContiguousPool(const T& initial_value, uint32_t initial_size = 32);

        ObjectId AddNewObject(const T& value = {});
        ObjectId AddNewObject(T&& value);

        T& Get(ObjectId object_id);
        const T& Get(ObjectId object_id) const;

        void RemoveObject(ObjectId object_id);

        [[nodiscard]] bool Contains(ObjectId object_id) const { return m_object_index_map.contains(object_id); }

        [[nodiscard]] size_t Size() const { return m_active_count; }

        T* Data() { return m_pool.data(); }

        iterator begin();
        iterator end();

        const_iterator cbegin();
        const_iterator cend();

    private:

        std::unordered_map<ObjectId, uint32_t> m_object_index_map;
        std::vector<T> m_pool;
        uint32_t m_active_count;
        ObjectId m_current_id;
    };

    template <typename T>
    ContiguousPool<T>::ContiguousPool(uint32_t initial_size)
    : m_pool(initial_size),
      m_active_count(0),
      m_current_id(0)
    {
    }

    template <typename T>
    ContiguousPool<T>::ContiguousPool(const T& initial_value, uint32_t initial_size)
    : m_pool(initial_size, initial_value),
      m_active_count(0),
      m_current_id(0)
    {
    }

    template <typename T>
    typename ContiguousPool<T>::ObjectId ContiguousPool<T>::AddNewObject(const T& value)
    {
        ObjectId new_id = m_current_id++;
        m_object_index_map.emplace(new_id, m_active_count);
        m_active_count++;
        if (m_active_count == m_pool.size())
        {
            m_pool.push_back(value);
        }
        else
        {
            m_pool[m_active_count - 1] = value;
        }
        return new_id;
    }

    template <typename T>
    typename ContiguousPool<T>::ObjectId ContiguousPool<T>::AddNewObject(T&& value)
    {
        ObjectId new_id = m_current_id++;
        m_object_index_map.emplace(new_id, m_active_count);
        m_active_count++;
        if (m_active_count == m_pool.size())
        {
            m_pool.push_back(std::move(value));
        }
        else
        {
            m_pool[m_active_count - 1] = std::move(value);
        }
        return new_id;
    }

    template <typename T>
    T& ContiguousPool<T>::Get(ObjectId object_id)
    {
        assert(m_object_index_map.contains(object_id) && "Need to pass valid ObjectId. Passed ObjectId does not exist is this ContiguousPool.");

        uint32_t index = m_object_index_map.at(object_id);
        return m_pool.at(index);
    }

    template <typename T>
    const T& ContiguousPool<T>::Get(ObjectId object_id) const
    {
        assert(m_object_index_map.contains(object_id) && "Need to pass valid ObjectId. Passed ObjectId does not exist is this ContiguousPool.");

        uint32_t index = m_object_index_map.at(object_id);
        return m_pool.at(index);
    }

    template <typename T>
    void ContiguousPool<T>::RemoveObject(ObjectId object_id)
    {
        assert(m_object_index_map.contains(object_id) && "Need to pass valid ObjectId. Passed ObjectId does not exist is this ContiguousPool.");

        const uint32_t last_index = m_active_count - 1;
        const uint32_t index = m_object_index_map.at(object_id);
        m_object_index_map.erase(object_id);

        assert(index < m_active_count && "Invalid index. something is wrong.");
        
        // Decrement active count (remove last)
        m_active_count--;
        if (index == last_index)
        {
            T removed_value = std::move(m_pool[index]);
            return;
        }

        // If removed object is not last element, switch position with element at the end.
        m_pool[index] = std::move(m_pool[last_index]);
        for (auto& iter : m_object_index_map)
        {
            if (iter.second == last_index)
            {
                m_object_index_map[iter.first] = index;
            }
        }
    }

    template <typename T>
    typename ContiguousPool<T>::iterator ContiguousPool<T>::begin()
    {
        return m_pool.begin();
    }

    template <typename T>
    typename ContiguousPool<T>::iterator ContiguousPool<T>::end()
    {
        return m_pool.begin() + m_active_count;
    }

    template <typename T>
    typename ContiguousPool<T>::const_iterator ContiguousPool<T>::cbegin()
    {
        return m_pool.cbegin();
    }

    template <typename T>
    typename ContiguousPool<T>::const_iterator ContiguousPool<T>::cend()
    {
        return m_pool.cbegin() + m_active_count;
    }
}

#endif