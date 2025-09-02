#ifndef BRR_DYNAMICPOOL_H
#define BRR_DYNAMICPOOL_H
#include <Core/LogSystem.h>

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
    template<typename KeyType, typename T, class Hasher = std::hash<KeyType>>
    class ContiguousPool
    {
    public:
        using iterator = typename std::vector<T>::iterator;
        using const_iterator = typename std::vector<T>::const_iterator;

        explicit ContiguousPool(uint32_t initial_size = 32);

        ContiguousPool(const T& initial_value, uint32_t initial_size = 32);

        bool AddObject(KeyType object_key, const T& value = {});
        bool AddObject(KeyType object_key, T&& value = {});

        T& Get(KeyType object_key);
        const T& Get(KeyType object_key) const;

        iterator Find(KeyType object_key);

        void RemoveObject(KeyType object_key);

        void Clear();

        [[nodiscard]] bool Contains(KeyType object_key) const { return m_object_index_map.contains(object_key); }

        [[nodiscard]] size_t Size() const { return m_active_count; }

        T* Data() { return m_pool.data(); }

        iterator begin();
        iterator end();

        const_iterator cbegin();
        const_iterator cend();

    private:

        std::unordered_map<KeyType, uint32_t, Hasher> m_object_index_map;
        std::vector<T> m_pool;
        uint32_t m_active_count;
    };

    template<typename KeyType, typename T, class Hasher>
    ContiguousPool<KeyType, T, Hasher>::ContiguousPool(uint32_t initial_size)
    : m_pool(initial_size),
      m_active_count(0)
    {
    }

    template<typename KeyType, typename T, class Hasher>
    ContiguousPool<KeyType, T, Hasher>::ContiguousPool(const T& initial_value, uint32_t initial_size)
    : m_pool(initial_size, initial_value),
      m_active_count(0)
    {
    }

    template<typename KeyType, typename T, class Hasher>
    bool ContiguousPool<KeyType, T, Hasher>::AddObject(KeyType object_key, const T& value)
    {
        if (m_object_index_map.contains(object_key))
        {
            BRR_LogError("Can't add object (ID: {}) to pool. Object with this ID already exists.", uint64_t(object_key));
            return false;
        }
        
        m_object_index_map.emplace(object_key, m_active_count);
        if (m_active_count == m_pool.size())
        {
            m_pool.push_back(value);
        }
        else
        {
            m_pool[m_active_count] = value;
        }
        m_active_count++;
        return true;
    }

    template<typename KeyType, typename T, class Hasher>
    bool ContiguousPool<KeyType, T, Hasher>::AddObject(KeyType object_key, T&& value)
    {
        if (m_object_index_map.contains(object_key))
        {
            BRR_LogError("Can't add object (ID: {}) to pool. Object with this ID already exists.", uint64_t(object_key));
            return false;
        }

        m_object_index_map.emplace(object_key, m_active_count);
        if (m_active_count == m_pool.size())
        {
            m_pool.push_back(std::move(value));
        }
        else
        {
            m_pool[m_active_count] = std::move(value);
        }
        m_active_count++;
        return true;
    }

    template<typename KeyType, typename T, class Hasher>
    T& ContiguousPool<KeyType, T, Hasher>::Get(KeyType object_key)
    {
        assert(m_object_index_map.contains(object_key) && "Need to pass valid ObjectId. Passed ObjectId does not exist is this ContiguousPool.");

        uint32_t index = m_object_index_map.at(object_key);
        return m_pool.at(index);
    }

    template<typename KeyType, typename T, class Hasher>
    const T& ContiguousPool<KeyType, T, Hasher>::Get(KeyType object_key) const
    {
        assert(m_object_index_map.contains(object_key) && "Need to pass valid ObjectId. Passed ObjectId does not exist is this ContiguousPool.");

        uint32_t index = m_object_index_map.at(object_key);
        return m_pool.at(index);
    }

    template <typename KeyType, typename T, class Hasher>
    typename ContiguousPool<KeyType, T, Hasher>::iterator ContiguousPool<KeyType, T, Hasher>::Find(KeyType object_key)
    {
        auto iter = m_object_index_map.find(object_key);
        if (iter == m_object_index_map.end())
        {
            return end();
        }

        return m_pool.begin() + iter->second;
    }

    template<typename KeyType, typename T, class Hasher>
    void ContiguousPool<KeyType, T, Hasher>::RemoveObject(KeyType object_key)
    {
        if (!m_object_index_map.contains(object_key))
        {
            BRR_LogError("Need to pass valid ObjectId. Passed ObjectId '{}' does not exist is this ContiguousPool.", uint64_t(object_key));
            return;
        }

        const uint32_t last_index = m_active_count - 1;
        const uint32_t index = m_object_index_map.at(object_key);
        m_object_index_map.erase(object_key);

        assert(index < m_active_count && "Invalid index. something is wrong.");
        
        m_active_count--;
        if (index == last_index)
        {
            T removed_value = std::move(m_pool[index]);
            return;
        }

        m_pool[index] = std::move(m_pool[last_index]);
        for (auto& iter : m_object_index_map)
        {
            if (iter.second == last_index)
            {
                m_object_index_map[iter.first] = index;
                break;
            }
        }
    }

    template <typename KeyType, typename T, class Hasher>
    void ContiguousPool<KeyType, T, Hasher>::Clear()
    {
        m_pool.clear();
        m_object_index_map.clear();
        m_active_count = 0;
    }

    template<typename KeyType, typename T, class Hasher>
    typename ContiguousPool<KeyType, T, Hasher>::iterator ContiguousPool<KeyType, T, Hasher>::begin()
    {
        return m_pool.begin();
    }

    template<typename KeyType, typename T, class Hasher>
    typename ContiguousPool<KeyType, T, Hasher>::iterator ContiguousPool<KeyType, T, Hasher>::end()
    {
        return m_pool.begin() + m_active_count;
    }

    template<typename KeyType, typename T, class Hasher>
    typename ContiguousPool<KeyType, T, Hasher>::const_iterator ContiguousPool<KeyType, T, Hasher>::cbegin()
    {
        return m_pool.cbegin();
    }

    template<typename KeyType, typename T, class Hasher>
    typename ContiguousPool<KeyType, T, Hasher>::const_iterator ContiguousPool<KeyType, T, Hasher>::cend()
    {
        return m_pool.cbegin() + m_active_count;
    }
}

#endif