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

    void LogSystem::SetPattern(const std::string& format_str)
    {
#ifdef USE_SPDLOG
        spdlog::set_pattern(format_str);
#else
        //TODO: Implement own
#endif
    }


    LogSystem::LogSystem()
    {
        
    }
}
