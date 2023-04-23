#include <Core/LogSystem.h>

#ifdef USE_SPDLOG
#include <spdlog/spdlog.h>
#endif

namespace brr
{
    LogSystem& LogSystem::Instance()
    {
        static LogSystem instance;
        return instance;
    }


    LogSystem::LogSystem()
    {
        
    }
}
