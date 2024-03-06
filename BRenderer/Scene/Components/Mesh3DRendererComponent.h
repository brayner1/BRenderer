#ifndef BRR_MESH3DRENDERERCOMPONENT_H
#define BRR_MESH3DRENDERERCOMPONENT_H

namespace brr
{
    class Mesh3DRendererComponent : public EntityComponent
    {
    public:
        Mesh3DRendererComponent(): m_id(0) {}

    private:

        uint32_t m_id;
    };
}

#endif