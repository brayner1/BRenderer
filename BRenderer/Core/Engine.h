#ifndef BRR_ENGINE_H
#define BRR_ENGINE_H

namespace brr
{
    namespace vis
    {
        class WindowManager;
    }
    class InputSystem;
    class Scene;

    class Engine
    {
    public:

        static bool IsInitialized();

        static vis::WindowManager* GetWindowManager();

        static InputSystem* GetInputSystem();

        static Scene* GetMainScene();

    private:
        friend class App;

        static void InitEngine();
        static void ShutdownEngine();

        static void UpdateMainLoop();
    };
}

#endif