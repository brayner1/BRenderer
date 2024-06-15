#ifndef BRR_WINDOWIMGUILAYER_H
#define BRR_WINDOWIMGUILAYER_H

namespace brr::vis
{
    class WindowImGuiLayer
    {
    public:
        virtual ~WindowImGuiLayer() = default;

        virtual void OnImGuiRender() {}
    };

}

#endif