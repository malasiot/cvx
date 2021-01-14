#ifndef CVX_UTIL_LOGGER_HPP
#define CVX_UTIL_LOGGER_HPP

#include <iostream>
#include <mutex>
#include <memory>
#include <cassert>
#include <vector>
#include <sstream>

namespace cvx {

// Context of application logging passed with its log message to the logger

struct LogContext
{
    LogContext(const char *file_name, int line_num, const char *context):
        file_(file_name), line_(line_num), context_(context), thread_id_(0) {}

    uint line_ ;
    const char *file_ ;
    const char *context_ ;
    int thread_id_ ;
} ;


enum LogLevel { Trace = 0, Debug = 1, Info = 2, Warning = 3, Error = 4, Fatal = 5 };

// Abstract formatter of messages
class LogFormatter {
public:

    LogFormatter() {}
    virtual ~LogFormatter() {}

    virtual std::string format(LogLevel level, const std::string &message, const LogContext *ctx) = 0 ;
};


// A logj style formater
class LogPatternFormatter: public LogFormatter
{
public:
    LogPatternFormatter(const std::string &pattern) ;

    /*
          The pattern is a format string with special
          flags:
                %v: log level
                %V: log level uppercase
                %c: function name
                %C: function name stripped
                %d{format}: Date with given format as given (default is ISO)
                %f: file path
                %F: file name
                %l: line number
                %r: time stamp (milliseconds from start of process)
                %t: thread ID
                %m: message string
                %%: prints %
           Optionally a format modifier may be inserted after %. This has the form [-][minLength][.maxLength]
           where - stand for left align (see log4j PatternLayout class documentation)
    */

    static const std::string DefaultFormat ; //"%d{%c} %f (%l) %c: %m";

protected:

    std::string format(LogLevel level, const std::string &message, const LogContext *ctx) override ;

private:

    ~LogPatternFormatter() override {}

    std::string pattern_ ;
};

// a simple formatter that disregards context and logging level

class LogSimpleFormatter: public LogFormatter {
public:
    LogSimpleFormatter() {}

    ~LogSimpleFormatter() override {}

protected:
    std::string format(LogLevel, const std::string &message, const LogContext *) override {
        return message ;
    }
};

// An appender sends a message to a device such as console or file
class LogSink {

public:
    LogSink(LogLevel levelThreshold, LogFormatter * formatter):
        threshold_(levelThreshold), formatter_(formatter) {
        assert(formatter_) ;
    }

    virtual ~LogSink() {}

    void setFormatter(LogFormatter *formatter) {
        formatter_.reset(formatter) ;
    }

    bool canAppend(LogLevel level) const {
        return level >= threshold_ ;
    }

    void setThreshold(LogLevel threshold) {
        threshold_ = threshold ;
    }

protected:

    std::string formattedMessage(LogLevel level, const std::string &message, const LogContext *ctx) {
        return formatter_->format(level, message, ctx) ;
    }


    friend class Logger ;

    virtual void append(LogLevel level, const std::string &message, const LogContext *ctx) = 0;

private:

    LogLevel threshold_ ;
    std::unique_ptr<LogFormatter> formatter_ ;
};

// Append to a stream object
class LogStreamSink: public LogSink {
public:
    LogStreamSink(LogLevel levelThreshold, LogFormatter *formatter, std::ostream &strm) ;
    ~LogStreamSink() override {
        strm_.flush() ;
    }

protected:

    void append(LogLevel level, const std::string &message, const LogContext *ctx) override ;
private:

    std::recursive_mutex mutex_;
    std::ostream &strm_ ;
};

// Append to file
class LogFileSink: public LogSink {
public:
    LogFileSink(LogLevel levelThreshold, LogFormatter *formatter,
                    const std::string &file_prefix, // path of file to write messages
                    size_t maxFileSize = 1024*1024, // max size of file after which rotation happens
                    int maxBackupFileIndex = 100,   // maximum number of rotated files to keep
                    bool append_ = true) ;          // append messages to current file instead of starting a new record for a new instance of the appender
    ~LogFileSink() override ;

protected:

    void append(LogLevel level, const std::string &message, const LogContext *ctx) override ;

private:

    unsigned int max_file_size_ ;
    int fd_ ;
    bool append_ ;
    std::string fileName_ ;
    int last_backup_file_index_, max_backup_index_;
};


class Logger ;

// Helper class for encapsulated a single formatted message and implement stream like log output
// This is designed to be used not directly but through logger interface bellow

class LoggerStream: public std::ostringstream
{
public:

    LoggerStream(Logger &logger, LogLevel level, const LogContext *ctx = nullptr): logger_(logger),
    level_(level), ctx_(ctx) {}

    LoggerStream(const LoggerStream& ls) :
        logger_(ls.logger_), level_(ls.level_), ctx_(ls.ctx_) {
    }

    ~LoggerStream();

private:

    Logger &logger_ ;
    LogLevel level_ ;
    const LogContext *ctx_ ;
} ;

// Main logger class. Forwards messages to sinks.

class Logger
{
public:

    Logger();

    // write a log message

    void write(LogLevel level, const LogContext &ctx, const std::string &msg) ;
    void write(LogLevel level, const std::string &msg) ;

    LoggerStream operator()(LogLevel level, const LogContext &ctx) {
        return LoggerStream(*this, level, &ctx);
    }

    LoggerStream operator()(LogLevel level) {
        return LoggerStream(*this, level, nullptr);
    }

    void addSink(LogSink *sink);

    // get global logger
    static Logger &instance() ;

    // set global logger
    static void setInstance(Logger *logger) {
        user_logger_.reset(logger) ;
    }

protected:

    friend class LoggerStream ;

    void writex(LogLevel level, const LogContext *ctx, const std::string &message) ;

    std::mutex lock_ ;
    std::vector<std::unique_ptr<LogSink>> sinks_ ;

    static std::unique_ptr<Logger> user_logger_ ;
};




#define LOG_X_STREAM(logger, level, msg) logger(level, cvx::LogContext(__FILE__, __LINE__, __FUNCTION__)) << msg ;
#define LOG_X_STREAM_IF(logger, level, condition, msg) if ( ! (condition) ) ; else LOG_X_STREAM(logger, level, msg) ;

#define LOG_X_STREAM_EVERY_N(logger, level, n, msg)\
do {\
  static int _log_occurences = 0, _log_occurences_mod_n = 0; \
  ++_log_occurences; \
  if ( ++_log_occurences_mod_n > n) _log_occurences_mod_n -= n; \
  if ( _log_occurences_mod_n == 1 ) \
    LOG_X_STREAM(logger, level, msg) ;\
} while (0) ;
#define LOG_X_STREAM_ONCE(logger, level, msg)\
do {\
  static bool _logged_already = false; \
  if ( !_logged_already ) \
    LOG_X_STREAM(logger, level, msg) ;\
  _logged_already = true ;\
} while (0) ;
#define LOG_X_STREAM_FIRST_N(logger, level, n, msg)\
do {\
    static int _log_occurences = 0; \
    if ( _log_occurences <= n ) ++_log_occurences ; \
    if ( _log_occurences <= n ) \
    LOG_X_STREAM(logger, level, msg) ;\
} while (0) ;

#define LOG_X_STREAM_EVERY_N_IF(logger, level, n, condition, msg)\
    if ( ! ( condition ) ) ; else LOG_X_STREAM_EVERY_N(logger, level, n, msg) ;
#define LOG_X_STREAM_ONCE_IF(logger, level, condition, msg)\
    if ( ! ( condition ) ) ; else LOG_X_STREAM_ONCE(logger, level, msg) ;
#define LOG_X_STREAM_FIRST_N_IF(logger, level, n, condition, msg)\
    if ( ! ( condition ) ) ; else LOG_X_STREAM_FIRST_N(logger, level, n, msg) ;
#define LOG_X_FORMAT_EVERY_N_IF(logger, level, n, condition, msg)\
    if ( ! ( condition ) ) ; else LOG_X_FORMAT_EVERY_N(logger, level, n, msg) ;
#define LOG_X_FORMAT_ONCE_IF(logger, level, condition, msg)\
    if ( ! ( condition ) ) ; else LOG_X_FORMAT_ONCE(logger, level, msg) ;
#define LOG_X_FORMAT_FIRST_N_IF(logger, level, n, condition, msg)\
    if ( ! ( condition ) ) ; else LOG_X_FORMAT_FIRST_N(logger, level, n, msg) ;

#ifndef NO_DEBUG_LOGGING
#define LOG_TRACE(msg) LOG_X_STREAM(cvx::Logger::instance(), cvx::Trace, msg)
#define LOG_TRACE_IF(condition, msg) LOG_X_STREAM_IF(cvx::Logger::instance(), cvx::Trace, condition, msg)

#define LOG_TRACE_EVERY_N(n, msg) LOG_X_STREAM_EVERY_N(cvx::Logger::instance(), cvx::Trace, n, msg)
#define LOG_TRACE_ONCE(msg) LOG_X_STREAM_ONCE(cvx::Logger::instance(), cvx::Trace, msg)
#define LOG_TRACE_FIRST_N(n, msg) LOG_X_STREAM_FIRST_N(cvx::Logger::instance(), cvx::Trace, n, msg)

#define LOG_TRACE_EVERY_N_IF(n, conditions, msg) LOG_X_STREAM_EVERY_N_IF(cvx::Logger::instance(), cvx::Trace, n, condition, msg)
#define LOG_TRACE_ONCE_IF(condition, msg) LOG_X_STREAM_ONCE_IF(cvx::Logger::instance(), cvx::Trace, condition, msg)
#define LOG_TRACE_FIRST_N_IF(n, condition, msg) LOG_X_STREAM_FIRST_N_IF(cvx::Logger::instance(), cvx::Trace, n, condition, msg)

#define LOG_DEBUG(msg) LOG_X_STREAM(cvx::Logger::instance(), cvx::Debug, msg)
#define LOG_DEBUG_IF(condition, msg) LOG_X_STREAM_IF(cvx::Logger::instance(), cvx::Debug, condition, msg)

#define LOG_DEBUG_EVERY_N(n, msg) LOG_X_STREAM_EVERY_N(cvx::Logger::instance(), cvx::Debug, n, msg)
#define LOG_DEBUG_ONCE(msg) LOG_X_STREAM_ONCE(cvx::Logger::instance(), cvx::Debug, msg)
#define LOG_DEBUG_FIRST_N(n, msg) LOG_X_STREAM_FIRST_N(cvx::Logger::instance(), cvx::Debug, n, msg)

#define LOG_DEBUG_EVERY_N_IF(n, conditions, msg) LOG_X_STREAM_EVERY_N_IF(cvx::Logger::instance(), cvx::Debug, n, condition, msg)
#define LOG_DEBUG_ONCE_IF(condition, msg) LOG_X_STREAM_ONCE_IF(cvx::Logger::instance(), cvx::Debug, condition, msg)
#define LOG_DEBUG_FIRST_N_IF(n, condition, msg) LOG_X_STREAM_FIRST_N_IF(cvx::Logger::instance(), cvx::Debug, n, condition, msg)
#else // debug and trace messages are compiled out
#define LOG_TRACE(msg)
#define LOG_TRACE_IF(condition, msg)

#define LOG_TRACE_EVERY_N(n, msg)
#define LOG_TRACE_ONCE(msg)
#define LOG_TRACE_FIRST_N(n, msg)
#define LOG_TRACE_EVERY_N_IF(n, conditions, msg)
#define LOG_TRACE_ONCE_IF(condition, msg)
#define LOG_TRACE_FIRST_N_IF(n, condition, msg)

#define LOG_DEBUG(msg)
#define LOG_DEBUG_IF(condition, msg)
#define LOG_DEBUG_EVERY_N(n, msg)
#define LOG_DEBUG_ONCE(msg)
#define LOG_DEBUG_FIRST_N(n, msg)
#define LOG_DEBUG_EVERY_N_IF(n, conditions, msg)
#define LOG_DEBUG_ONCE_IF(condition, msg)
#define LOG_DEBUG_FIRST_N_IF(n, condition, msg)
#endif

#define LOG_INFO(msg) LOG_X_STREAM(cvx::Logger::instance(), cvx::Info, msg)
#define LOG_INFO_IF(condition, msg) LOG_X_STREAM_IF(cvx::Logger::instance(), cvx::Info, condition, msg)
#define LOG_INFO_EVERY_N(n, msg) LOG_X_STREAM_EVERY_N(cvx::Logger::instance(), cvx::Info, n, msg)
#define LOG_INFO_ONCE(msg) LOG_X_STREAM_ONCE(cvx::Logger::instance(), cvx::Info, msg)
#define LOG_FIRST_N(n, msg) LOG_X_STREAM_FIRST_N(cvx::Logger::instance(), cvx::Info, n, msg)
#define LOG_INFO_EVERY_N_IF(n, conditions, msg) LOG_X_STREAM_EVERY_N_IF(cvx::Logger::instance(), cvx::Info, n, condition, msg)
#define LOG_INFO_ONCE_IF(condition, msg) LOG_X_STREAM_ONCE_IF(cvx::Logger::instance(), cvx::Info, condition, msg)
#define LOG_INFO_FIRST_N_IF(n, condition, msg) LOG_X_STREAM_FIRST_N_IF(cvx::Logger::instance(), cvx::Info, n, condition, msg)

#define LOG_WARN(msg) LOG_X_STREAM(cvx::Logger::instance(), cvx::Warning, msg)
#define LOG_WARN_IF(condition, msg) LOG_X_STREAM_IF(cvx::Logger::instance(), cvx::Warning, condition, msg)
#define LOG_WARN_VERY_N(n, msg) LOG_X_STREAM_EVERY_N(cvx::Logger::instance(), cvx::Warning, n, msg)
#define LOG_WARN_ONCE(msg) LOG_X_STREAM_ONCE(cvx::Logger::instance(), cvx::Warning, msg)
#define LOG_WARN_FIRST_N(n, msg) LOG_X_STREAM_FIRST_N(cvx::Logger::instance(), cvx::Warning, n, msg)
#define LOG_WARN_EVERY_N_IF(n, conditions, msg) LOG_X_STREAM_EVERY_N_IF(cvx::Logger::instance(), cvx::Warning, n, condition, msg)
#define LOG_WARN_ONCE_IF(condition, msg) LOG_X_STREAM_ONCE_IF(cvx::Logger::instance(), cvx::Warning, condition, msg)
#define LOG_WARN_FIRST_N_IF(n, condition, msg) LOG_X_STREAM_FIRST_N_IF(cvx::Logger::instance(), cvx::Warning, n, condition, msg)

#define LOG_ERROR(msg) LOG_X_STREAM(cvx::Logger::instance(), cvx::Error, msg)
#define LOG_ERROR_IF(condition, msg) LOG_X_STREAM_IF(cvx::Logger::instance(), cvx::Error, condition, msg)
#define LOG_ERROR_EVERY_N(n, msg) LOG_X_STREAM_EVERY_N(cvx::Logger::instance(), cvx::Error, n, msg)
#define LOG_ERROR_ONCE(msg) LOG_X_STREAM_ONCE(cvx::Logger::instance(), cvx::Error, msg)
#define LOG_ERROR_FIRST_N(n, msg) LOG_X_STREAM_FIRST_N(cvx::Logger::instance(), cvx::Error, n, msg)
#define LOG_ERROR_EVERY_N_IF(n, conditions, msg) LOG_X_STREAM_EVERY_N_IF(cvx::Logger::instance(), cvx::Error, n, condition, msg)
#define LOG_ERROR_ONCE_IF(condition, msg) LOG_X_STREAM_ONCE_IF(cvx::Logger::instance(), cvx::Error, condition, msg)
#define LOG_ERROR_FIRST_N_IF(n, condition, msg) LOG_X_STREAM_FIRST_N_IF(cvx::Logger::instance(), cvx::Error, n, condition, msg)

#define LOG_FATAL(msg) LOG_X_STREAM(cvx::Logger::instance(), cvx::Fatal, msg)
#define LOG_FATAL_IF(condition, msg) LOG_X_STREAM_IF(cvx::Logger::instance(), cvx::Fatal, condition, msg)
#define LOG_FATAL_EVERY_N(n, msg) LOG_X_STREAM_EVERY_N(cvx::Logger::instance(), cvx::Fatal, n, msg)
#define LOG_FATAL_ONCE(msg) LOG_X_STREAM_ONCE(cvx::Logger::instance(), cvx::Fatal, msg)
#define LOG_FATAL_FIRST_N(n, msg) LOG_X_STREAM_FIRST_N(cvx::Logger::instance(), cvx::Fatal, n, msg)
#define LOG_FATAL_EVERY_N_IF(n, conditions, msg) LOG_X_STREAM_EVERY_N_IF(cvx::Logger::instance(), cvx::Fatal, n, condition, msg)
#define LOG_FATAL_ONCE_IF(condition, msg) LOG_X_STREAM_ONCE_IF(cvx::Logger::instance(), cvx::Fatal, condition, msg)
#define LOG_FATAL_FIRST_N_IF(n, condition, msg) LOG_X_STREAM_FIRST_N_IF(cvx::Logger::instance(), cvx::Fatal, n, condition, msg)


} // namespace cvx


#endif
