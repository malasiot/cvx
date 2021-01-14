#include <cvx/misc/logger.hpp>
#include <cvx/misc/path.hpp>
#include <cvx/misc/strings.hpp>
#include <cvx/misc/zstream.hpp>

#include <iostream>

#include <limits.h>
#include <stdio.h>

#include <cstdarg>
#include <fstream>

#include <fcntl.h>
#include <cstdio>

using namespace std;

namespace cvx {

const std::string LogPatternFormatter::DefaultFormat = "%d{%c} %f (%l) %c: %m";

static string levelToString(LogLevel ltype)
{
    switch (ltype)
    {
        case Error:
            return "Error";
        case Warning:
            return "Warning";
        case Debug:
            return "Debug";
        case Info:
            return "Info";
        case Trace:
            return "Trace";
        default:
            return string();
    }
}

#define LITERAL_STATE 0
#define CONVERTER_STATE 1
#define DOT_STATE 2
#define MIN_STATE 3
#define MAX_STATE 4
#define COMMAND_STATE 5

static void printFormatedString(std::ostream &strm, const string &buffer, bool leftAlign, int minLength, int maxLength)
{
    int rawLength = buffer.length() ;

    if ( rawLength > maxLength ) {
        strm.write(buffer.c_str() + rawLength - maxLength, maxLength) ;
    } else if (rawLength < minLength) {
        if ( leftAlign ) {
            strm.write(buffer.c_str(), rawLength) ;
            for(int i=0 ; i<minLength - rawLength ; i++ ) strm.put(' ') ;
        } else {

            for(int i=0 ; i<minLength - rawLength ; i++ ) strm.put(' ') ;
            strm.write(buffer.c_str(), rawLength) ;
        }
    }
    else strm.write(buffer.c_str(), rawLength) ;

}

static void formatMessage(std::ostream &strm, const string &pattern, LogLevel level, const string &message, int threadId, const string &fileName = string(), int currentLine = 0,
                               const string &context = string())

{
    int state = LITERAL_STATE;
    int i = 0;
    int patternLength = pattern.length() ;
    bool leftAlign ;
    int minLength, maxLength ;

    while ( i < patternLength )
    {
        char c = pattern[i++];

        switch (state)
        {
            case LITERAL_STATE:
                leftAlign = false ;
                minLength = 0 ;
                maxLength = INT_MAX ;

                // In literal state, the last char is always a literal.
                if (i == patternLength) {
                    strm.put(c) ;
                    continue;
                }

                if ( c == '%' ) {
                    // peek at the next char.
                    if ( pattern[i] == '%' ) {
                        strm.put(c) ;
                        i++;
                    } else {
                        state = CONVERTER_STATE;
                    }
                } else {
                    strm.put(c);
                }

                break;

            case CONVERTER_STATE:
                switch (c)
                {
                    case  '-':
                        leftAlign = true ;
                        break;
                    case '.':
                        state = DOT_STATE;
                        break;
                    default:
                        if ((c >= '0') && (c <= '9'))
                        {
                            state = MIN_STATE ;
                            minLength = c - '0' ;
                        } else {
                            --i ;
                            state = COMMAND_STATE;
                        }
                }

                break;

            case MIN_STATE:
                if ((c >= '0') && (c <= '9'))
                {
                    minLength = minLength * 10 + c - '0' ;
                }
                else if (c == '.')
                {
                    state = DOT_STATE;
                } else {
                    --i ;
                    state = COMMAND_STATE;
                }

                break;

            case DOT_STATE:
                if ((c >= '0') && (c <= '9'))
                {
                    maxLength = c  - '0' ;
                    state = MAX_STATE;
                } else {
                    //  Error in pattern, was expecting digit.";
                    strm.put(c) ;
                    state = LITERAL_STATE;
                }
                break;

            case MAX_STATE:
                if ((c >= '0') && (c <= '9')) {
                    maxLength = maxLength * 10 + c - '0' ;
                }
                else {
                    --i ;
                    state = COMMAND_STATE;
                }
                break;

            case COMMAND_STATE:
                switch (c)
                {
                case 'v': //log level
                {
                    string levelStr = levelToString(level) ;
                    printFormatedString(strm, levelStr, leftAlign, minLength, maxLength) ;
                    break ;
                }
                case 'V': //log level uppercase
                {
                    string levelStr = levelToString(level) ;
                    printFormatedString(strm, toUpperCopy(levelStr), leftAlign, minLength, maxLength) ;
                    break ;
                }
                case 'c': //function name
                    if ( !context.empty() )
                            printFormatedString(strm, context, leftAlign, minLength, maxLength) ;
                            break ;
                case 'f': // %f: file path
                    if ( !fileName.empty() )
                        printFormatedString(strm, fileName, leftAlign, minLength, maxLength) ;
                    break ;
                case 'F': // %F: file name
                    if ( !fileName.empty() )
                    {
                        string file_name_ = Path(fileName).name() ;
                        printFormatedString(strm, file_name_, leftAlign, minLength, maxLength) ;
                    }
                    break ;

                case 'l': // %l: line number
                    {
                        if ( currentLine )
                        {
                            std::ostringstream sb ;
                            sb << currentLine ;
                            printFormatedString(strm, sb.str().c_str(), leftAlign, minLength, maxLength) ;
                        }
                    }
                    break ;
                    case 'd': //%d{format}: Date with given format as given (default is %H:%M:%S)
                        {
                            struct tm *newtime ;
                            time_t ltime ;
                            time(&ltime) ;

                            //newtime = gmtime( &ltime ); // C4996
                            newtime = localtime(&ltime) ;

                            char buf[80] ;
                            int bufLen ;

                            if ( pattern[i] == '{' )
                            {
                                char format[80], *p = format ;

                                while ( pattern[++i] != '}' ) *p++ = pattern[i] ;
                                *p = 0 ; ++i ;

                                bufLen = strftime(buf, 80, format, newtime) ;

                            }
                            else
                                bufLen = strftime(buf, 80, "%H:%M:%S", newtime) ;

                            strm.write(buf, bufLen) ;


                        }

                        break ;

                    case 'r': // time stamp
                        {
                            unsigned long cc = clock() * 1000.0 / CLOCKS_PER_SEC ;

                            std::ostringstream sb ;
                            sb << cc ;
                            printFormatedString(strm, sb.str(), leftAlign, minLength, maxLength) ;
                        }
                        break ;
                    case 't': // thread ID
                        {
                            unsigned long cc = threadId ;

                            std::ostringstream sb ;
                            sb << cc ;
                            printFormatedString(strm, sb.str(), leftAlign, minLength, maxLength) ;
                        }
                        break ;
                    case 'm': // message
                        printFormatedString(strm, message, leftAlign, minLength, maxLength) ;
                        break ;

                    default:
                        //  Error in pattern, invalid command specifier ;
                        strm.put(c) ;
                        state = LITERAL_STATE;
                        continue ;

                }
                state = LITERAL_STATE ;
                break ;
        } // switch
    }


}

LogPatternFormatter::LogPatternFormatter(const string &pattern): LogFormatter(), pattern_(pattern) {

}

string LogPatternFormatter::format(LogLevel level, const string &message, const LogContext *ctx) {
    stringstream strm ;

    if ( ctx )
        formatMessage(strm, pattern_, level, message, ctx->thread_id_, ctx->file_, ctx->line_, ctx->context_) ;
    else
        formatMessage(strm, pattern_, level, message, 0, "", 0, "") ;

    return strm.str() ;
}

////////////////////////////////////////////////////////////////////////////////
//  Logger
////////////////////////////////////////////////////////////////////////////////

Logger::Logger() {}

void Logger::write(LogLevel level, const LogContext &ctx, const std::string &msg) {
    writex(level, &ctx, msg) ;
}

void Logger::write(LogLevel level, const std::string &msg) {
    writex(level, nullptr, msg) ;
}


void Logger::addSink(LogSink *sink)
{
    lock_guard<mutex> lock(lock_);

    sinks_.push_back(std::unique_ptr<LogSink>(sink)) ;
}



void Logger::writex(LogLevel level, const LogContext *ctx, const string &message)
{
    for( size_t i=0 ; i<sinks_.size() ; i++ )
        sinks_[i]->append(level, message, ctx) ;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////

LogStreamSink::LogStreamSink(LogLevel levelThreshold, LogFormatter *formatter, ostream &strm):
    LogSink(levelThreshold, formatter), strm_(strm) {

}

void LogStreamSink::append(LogLevel level, const string &message, const LogContext *ctx) {
    if ( canAppend(level) ) {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        strm_ << formattedMessage(level, message, ctx) << endl ;
    }
}

#define MAX_BACKUP_INDEX 20

LogFileSink::LogFileSink(LogLevel levelThreshold, LogFormatter *formatter,
                                 const string &fileName, size_t maxFileSize, int maxBackupIndex, bool _append): LogSink(levelThreshold, formatter), fileName_(fileName),
    max_file_size_(maxFileSize), max_backup_index_(maxBackupIndex), append_(_append)
{
    unsigned int flags = O_CREAT | O_APPEND | O_WRONLY ;
    if ( !append_ ) flags |= O_TRUNC;

    fd_ = ::open(fileName.c_str(), flags, 00644);

    // find the index of the last backup file

    int last ;
    for( last=0 ; last <= MAX_BACKUP_INDEX ; last++ )
    {
        stringstream stream ;
        stream << fileName_ << '.' << last+1 << ".gz" ;
        string log_file = stream.str() ;
        if ( Path::exists(log_file) ) break ;
    }

    last_backup_file_index_ = last ;
}

LogFileSink::~LogFileSink()
{
    ::close(fd_) ;
}


static bool compress(const string &srcFile, const string &outFile)
{
    ifstream istrm(srcFile.c_str(), ios_base::in);
    ozfstream ozstrm(outFile.c_str(), ios::out | ios::binary) ;

    char buffer[4096];
    while (istrm.read(buffer, sizeof(buffer)))
        ozstrm.write(buffer, sizeof(buffer));

    return ozstrm.good() ;
}


static string makeOutFilename(const string &name, uint num) {
    stringstream stream ;
    stream << name << '.' << num << ".gz" ;
    return stream.str() ;
}

void LogFileSink::append(LogLevel level, const string &message, const LogContext *ctx)
{
    if ( !canAppend(level) ) return ;

    string s = formattedMessage(level, message, ctx) + '\n';
    ::write(fd_, s.data(), s.length()) ;

    off_t offset = ::lseek(fd_, 0, SEEK_END);
    if ( offset < 0 || static_cast<size_t>(offset) < max_file_size_ ) return ;

    // backup current file and open a new one

    ::close(fd_);

    // remove last file if too many backup files

    string last_log_file = makeOutFilename(fileName_, last_backup_file_index_) ;

    if ( last_backup_file_index_ == max_backup_index_ )
    {
        Path::remove(last_log_file) ;
        last_backup_file_index_ -- ;
    }

    // rename old backup files

    last_log_file = makeOutFilename(fileName_, last_backup_file_index_ + 1) ;

    for( int i=last_backup_file_index_ ; i>=1; i-- )
    {
        string log_file = makeOutFilename(fileName_, i) ;
        Path::rename(log_file, last_log_file) ;
        last_log_file = log_file ;
    }

    // backup and compress current file

    compress(fileName_, fileName_ + ".1.gz") ;
    Path::remove(fileName_) ;

    last_backup_file_index_ ++ ;

    // open new file

    unsigned int flags = O_CREAT | O_APPEND | O_WRONLY ;
    if ( !append_ ) flags |= O_TRUNC;

    fd_ = ::open(fileName_.c_str(), flags, 00644);
}


#ifdef _MSC_VER

class MSVCAppender: public LogAppender
{
public:
    MSVCAppender(LogLevel levelThreshold, LogFormatter *formatter): LogAppender(levelThreshold, formatter) {}

protected:

    friend class Logger ;
    virtual void append(LogLevel level, const LogContext &ctx, const std::string &message)
    {
        string s = this->formattedMessage(level, ctx, message) ;
        ::OutputDebugStringA(s.c_str()) ;
    }
} ;

class DefaultLogger: public Logger
{
public:
    DefaultLogger() {
        addAppender(new MSVCAppender(Trace, new LogPatternFormatter("%In function %c, %F:%l: %m"))) ;
    }

};


#else
class DefaultLogger: public Logger
{
public:
    DefaultLogger() {
        LogPatternFormatter *f = new LogPatternFormatter("%In function %c, %F:%l: %m") ;
        LogStreamSink *a = new LogStreamSink(Trace, f, std::cout) ;
        addSink(a) ;
    }
};
#endif

std::unique_ptr<Logger> Logger::user_logger_ ;

Logger &Logger::instance() {
    static DefaultLogger default_logger_ ;
    if ( !user_logger_ ) return default_logger_ ;
    else return *user_logger_ ;
}

LoggerStream::~LoggerStream()  {
    logger_.writex(level_, ctx_, str()) ;
}

}

