#pragma once

#include <cvx/misc/variant.hpp>

#include <istream>
#include <memory>

namespace cvx {

namespace detail {
class JSONTokenizer ;
}

class Config {
public:
    Config(std::istream &strm) ;
    ~Config() ;

private:

    struct ParseContext {
        std::unique_ptr<detail::JSONTokenizer> tokenizer_ ;
        Variant var_ ;
        int token_ ;
    };

    Variant container_ ;

    void parse(ParseContext &ctx, Variant &v) ;
    void parseNameValue(ParseContext &ctx, Variant &v);
    Variant parseValue(ParseContext &ctx) ;
};



}
