#ifndef BRR_LOGSYSTEM_H
#define BRR_LOGSYSTEM_H
#include <string>
#include <sstream>

namespace brr
{
    enum class LogLevel : int;

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

    class LogStreamBuffer
    {
    public:
        LogStreamBuffer(LogLevel level, const char* filename_in, int line_in, const char* funcname_in);

        ~LogStreamBuffer();

        void Flush();

        template<typename T>
        LogStreamBuffer& operator<< (T&& input)
        {
            message << input;
            return *this;
        }

    private:
        std::stringstream message;
        LogLevel          log_level;
        const char*       filename;
        int               line;
        const char*       funcname;
    };

    // TODO: Define interfaces here. Implement class on library-specific file?
    class LogSystem
    {
    public:

        static LogSystem& Instance();

        /**
         * Set the pattern of the logging message.
         * Use the following pattern flags:
         - |'%v'|   The actual text to log|"some user text"|
         - |'%t'|   Thread id|"1232"|
         - |'%P'|   Process id|"3456"|
         - |'%n'|   Logger's name|"some logger name"
         - |'%l'|   The log level of the message|"debug", "info", etc|
         - |'%L'|   Short log level of the message|"D", "I", etc|
         - |'%a'|   Abbreviated weekday name|"Thu"|
         - |'%A'|   Full weekday name|"Thursday"|
         - |'%b'|   Abbreviated month name|"Aug"|
         - |'%B'|   Full month name|"August"|
         - |'%c'|   Date and time representation|"Thu Aug 23 15:35:46 2014"|
         - |'%C'|   Year in 2 digits|"14"|
         - |'%Y'|   Year in 4 digits|"2014"|
         - |'%D' or `%x`|   Short MM/DD/YY date|"08/23/14"|
         - |'%m'|   Month 01-12|"11"|
         - |'%d'|   Day of month 01-31|"29"|
         - |'%H'|   Hours in 24 format  00-23|"23"|
         - |'%I'|   Hours in 12 format  01-12|"11"|
         - |'%M'|   Minutes 00-59|"59"|
         - |'%S'|   Seconds 00-59|"58"|
         - |'%e'|   Millisecond part of the current second 000-999|"678"|
         - |'%f'|   Microsecond part of the current second 000000-999999|"056789"|
         - |'%F'|   Nanosecond part of the current second 000000000-999999999|"256789123"|
         - |'%p'|   AM/PM|"AM"|
         - |'%r'|   12 hour clock|"02:55:02 PM"|
         - |'%R'|   24-hour HH:MM time, equivalent to %H:%M|"23:55"|
         - |'%T' or `%X`|   ISO 8601 time format (HH:MM:SS), equivalent to %H:%M:%S|"23:55:59"|
         - |'%z'|   ISO 8601 offset from UTC in timezone ([+/-]HH:MM)|"+02:00"|
         - |'%E'|   Seconds since the epoch |"1528834770"|
         - |'%%'|   The % sign|"%"|
         - |'%+'|   spdlog's default format|"[2014-10-31 23:46:59.678] [mylogger] [info] Some message"|
         - |'%^'|   start color range (can be used only once)|"[mylogger] [info(green)] Some message"|
         - |'%$'|   end color range (for example %^[+++]%$ %v) (can be used only once)|[+++] Some message|
         - |'%@'|   Source file and line (use SPDLOG_TRACE(..), SPDLOG_INFO(...) etc. instead of spdlog::trace(...)) Same as %g:%#|/some/dir/my_file.cpp:123|
         - |'%s'|   Basename of the source file (use SPDLOG_TRACE(..), SPDLOG_INFO(...) etc.)|my_file.cpp|
         - |'%g'|   Full or relative path of the source file as appears in the `__FILE__` macro (use SPDLOG_TRACE(..), SPDLOG_INFO(...) etc.)|/some/dir/my_file.cpp|
         - |'%#'|   Source line (use SPDLOG_TRACE(..), SPDLOG_INFO(...) etc.)|123|
         - |'%!'|   Source function (use SPDLOG_TRACE(..), SPDLOG_INFO(...) etc. see tweakme for pretty-print)|my_func|
         - |'%o'|   Elapsed time in milliseconds since previous message|456|
         - |'%i'|   Elapsed time in microseconds since previous message|456|
         - |'%u'|   Elapsed time in nanoseconds since previous message|11456|
         - |'%O'|   Elapsed time in seconds since previous message|4|
         */
        static void SetPattern(const std::string& format_str);

    protected:
        LogSystem();
    };

    
}

#ifdef USE_SPDLOG
#include <spdlog/spdlog.h>

#define BRR_LogTrace(...)       (SPDLOG_TRACE(__VA_ARGS__))
#define BRR_LogDebug(...)       (SPDLOG_DEBUG(__VA_ARGS__))
#define BRR_LogInfo(...)        (SPDLOG_INFO(__VA_ARGS__))
#define BRR_LogWarn(...)        (SPDLOG_WARN(__VA_ARGS__))
#define BRR_LogError(...)       (SPDLOG_ERROR(__VA_ARGS__))
#define BRR_LogCritical(...)    (SPDLOG_CRITICAL(__VA_ARGS__))

namespace brr
{
    enum class LogLevel : int {
        Trace    = SPDLOG_LEVEL_TRACE,
        Debug    = SPDLOG_LEVEL_DEBUG,
        Info     = SPDLOG_LEVEL_INFO,
        Warning  = SPDLOG_LEVEL_WARN,
        Error    = SPDLOG_LEVEL_ERROR,
        Critical = SPDLOG_LEVEL_CRITICAL
    };
}

#else

#define BRR_LogTrace(...)       void(0)
#define BRR_LogDebug(...)       void(0)
#define BRR_LogInfo(...)        void(0)
#define BRR_LogWarn(...)        void(0)
#define BRR_LogError(...)       void(0)
#define BRR_LogCritical(...)    void(0)

enum class LogLevel : int {
    Trace    = 0,
    Debug    = 1,
    Info     = 2,
    Warning  = 3,
    Error    = 4,
    Critical = 5
};

#endif

#define BRR_TraceStrBuff()        (LogStreamBuffer(LogLevel::Trace,    __FILE__, __LINE__, __FUNCTION__))
#define BRR_DebugStrBuff()        (LogStreamBuffer(LogLevel::Debug,    __FILE__, __LINE__, __FUNCTION__))
#define BRR_InfoStrBuff()         (LogStreamBuffer(LogLevel::Info,     __FILE__, __LINE__, __FUNCTION__))
#define BRR_WarnStrBuff()         (LogStreamBuffer(LogLevel::Warning,  __FILE__, __LINE__, __FUNCTION__))
#define BRR_ErrorStrBuff()        (LogStreamBuffer(LogLevel::Error,    __FILE__, __LINE__, __FUNCTION__))
#define BRR_CriticalStrBuff()     (LogStreamBuffer(LogLevel::Critical, __FILE__, __LINE__, __FUNCTION__))

#endif