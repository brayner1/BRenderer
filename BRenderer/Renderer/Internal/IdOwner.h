#ifndef BRR_IDOWNER_H
#define BRR_IDOWNER_H

namespace brr::render::internal
{
    template <typename IdType>
    class IdOwner
    {
    public:
        IdType GetNewId()
        {
            return m_current_id++;
        }

    private:
        std::atomic<IdType> m_current_id = 0;
    };
}

#endif
