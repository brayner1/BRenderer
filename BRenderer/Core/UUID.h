#ifndef BRR_UUID_H
#define BRR_UUID_H

#include <cstdint>

namespace brr
{
    class UUID
    {
    public:

        UUID();
		UUID(uint64_t uuid);
		UUID(const UUID&) = default;

        operator uint64_t() const { return m_uuid; }

    private:

        uint64_t m_uuid;
    };
}

namespace std {
	template <typename T> struct hash;

	template<>
	struct hash<brr::UUID>
	{
		size_t operator()(const brr::UUID& uuid) const noexcept
        {
			return uuid;
		}
	};

}

#endif
