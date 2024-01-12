#include "LogSystem.h"

#ifdef USE_SPDLOG
#include <spdlog/spdlog.h>
#endif

namespace brr
{
    LogStreamBuffer::LogStreamBuffer(LogLevel level, const char* filename_in, int line_in, const char* funcname_in)
    :   log_level(level),
        filename(filename_in),
        line(line_in),
        funcname(funcname_in)
    {
    }

    LogStreamBuffer::~LogStreamBuffer()
    {
        Flush();
    }

    void LogStreamBuffer::Flush()
    {
        if (message.str().empty())
        {
            return;
        }
#ifdef USE_SPDLOG
        spdlog::log(
            spdlog::source_loc{ filename, line, funcname },
            static_cast<spdlog::level::level_enum>(log_level),
            message.str()
        );
#endif
        message.str({});
        message.clear();
    }

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

    void LogSystem::SetLevel(LogLevel logLevel)
    {
#ifdef USE_SPDLOG
        spdlog::level::level_enum spdlogLevel = static_cast<spdlog::level::level_enum>(logLevel);
        spdlog::set_level(spdlogLevel);
#endif
    }


    LogSystem::LogSystem()
    {
    }
}
