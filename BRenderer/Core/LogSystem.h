#ifndef BRR_LOGSYSTEM_H
#define BRR_LOGSYSTEM_H

#ifdef USE_SPDLOG
#include <spdlog/spdlog.h>

#define BRR_LogTrace(...)       (SPDLOG_TRACE(__VA_ARGS__))
#define BRR_LogDebug(...)       (SPDLOG_DEBUG(__VA_ARGS__))
#define BRR_LogInfo(...)        (SPDLOG_INFO(__VA_ARGS__))
#define BRR_LogWarn(...)        (SPDLOG_WARN(__VA_ARGS__))
#define BRR_LogError(...)       (SPDLOG_ERROR(__VA_ARGS__))
#define BRR_LogCritical(...)    (SPDLOG_CRITICAL(__VA_ARGS__))

#else

#define BRR_LogTrace(...)       void(0)
#define BRR_LogDebug(...)       void(0)
#define BRR_LogInfo(...)        void(0)
#define BRR_LogWarn(...)        void(0)
#define BRR_LogError(...)       void(0)
#define BRR_LogCritical(...)    void(0)

#endif

namespace brr
{
    struct Logger {};

    struct StdoutLogger : Logger {};

    struct FileLogger : Logger
    {
        std::string filename;
    };

    struct RotatingFileLogger : Logger
    {
        std::string filename;
        size_t      file_size;
        size_t      max_files;
    };

    struct DailyFileLogger : Logger
    {
        std::string filename;
        uint8_t     change_hour;
        uint8_t     change_minute;
    };

    // TODO: Define interfaces here. Implement class on library-specific file?
    class LogSystem
    {
    public:

        static LogSystem& Instance();

        void AddLogger(const Logger* logger);

    protected:
        LogSystem();
    };

    
}

#endif