#pragma once

#include <cvx/misc/variant.hpp>

#include <istream>
#include <memory>

namespace cvx {

namespace detail {
class JSONTokenizer ;
}

class ConfigParser {

    ConfigParser() = default ;
    ~ConfigParser() ;

public:
    static Variant loadFile(const std::string &fpath, const std::string &inc_path = {}) ;
    static Variant loadString(const std::string &str,  const std::string &inc_path = {}) ;

private:

    struct ParseContext {
        std::unique_ptr<detail::JSONTokenizer> tokenizer_ ;
        Variant var_ ;
        std::string inc_path_, path_ ;
    };

    static void parse(ParseContext &ctx, Variant &v) ;
    static bool parseNameValue(ParseContext &ctx, Variant &v);
    static Variant parseValue(ParseContext &ctx) ;
};

class ConfigParseException: public std::runtime_error {
public:
    ConfigParseException(const std::string &msg): std::runtime_error(msg) {}
};


}
