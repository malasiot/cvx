#ifndef CVX_UTIL_FORMAT_HPP
#define CVX_UTIL_FORMAT_HPP

#include <cvx/misc/detail/formatter.hpp>
#include <cvx/misc/variant.hpp>
#include <sstream>

// type safe string formating ala printf

namespace cvx {

namespace detail {
    struct FormatPart ;
}

// compiled format

class Format {
public:
    Format(const char *fmt) ;
    void format(std::ostream &strm, detail::FormatArg args[], size_t n_args) const ;

private:

    void parse() ;

    const char *fmt_ ;
    std::vector<detail::FormatPart> parts_ ;
};

class FormatParseException: public std::runtime_error {
  public:
    FormatParseException(const std::string &msg): std::runtime_error(msg) {}
};

class FormatArgException: public std::runtime_error {
  public:
    FormatArgException(const std::string &msg): std::runtime_error(msg) {}
};


template<typename ...Args>
void format(std::ostream &strm, const Format &fmt, const Args&... args ) {
   detail::FormatArg arg_list[] = { detail::FormatArg(args)... } ;
   fmt.format(strm, arg_list, sizeof...(args)) ;
}

template<typename ...Args>
void format(std::ostream &strm, const char *fmt_str, const Args&... args ) {
   Format fmt(fmt_str) ;
   format(strm, fmt, args...) ;
}

template<typename ...Args>
std::string format(const char *fmt_str, const Args&... args ) {
   std::ostringstream strm ;
   format(strm, fmt_str, args...) ;
   return strm.str() ;
}

template<typename ...Args>
std::string format(const Format &fmt, const Args&... args ) {
   std::ostringstream strm ;
   format(strm, fmt, args...) ;
   return strm.str() ;
}


}

#endif
