#include <cvx/misc/config.hpp>

#include "json_tokenizer.hpp"

#include <iostream>
#include <fstream>

using namespace std ;

namespace cvx {

Config::Config() {
    container_ = Variant::Object() ;
}

Config::~Config(){
}

void Config::loadFile(const std::string &fpath, const std::string &inc_path) {
    using namespace detail ;

    ifstream strm(fpath) ;
    if ( !strm ) throw ConfigParseException("Cannot load config file: " + fpath) ;

    ParseContext ctx ;

    ctx.tokenizer_.reset(new JSONTokenizer(strm, true)) ;
    ctx.inc_path_ = inc_path ;
    ctx.path_ = fpath ;

    try {
        parse(ctx, container_) ;
        container_.toJSON(std::cout);
    } catch ( JSONTokenizerParseException &e ) {
        string msg = "Error while parsing: " + fpath + "\n" ;
        msg += e.what() ;
        throw ConfigParseException(msg) ;
    }
}

void Config::loadString(const std::string &src, const std::string &inc_path) {
    using namespace detail ;

    istringstream strm(src) ;

    ParseContext ctx ;

    ctx.tokenizer_.reset(new JSONTokenizer(strm, true)) ;
    ctx.inc_path_ = inc_path ;

    try {
        parse(ctx, container_) ;
        container_.toJSON(std::cout);
    } catch ( JSONTokenizerParseException &e ) {
        string msg = "Error while parsing from string\n" ;
        msg += e.what() ;
        throw ConfigParseException(msg) ;
    }
}

bool Config::hasKey(const std::string &path) {
    Variant val ;
    return ( container_.lookup(path, val) ) ;
}

bool Config::value(const char *path, std::string &value) const {
    Variant val ;
    if ( !container_.lookup(path, val) ) return false ;
    if ( !val.isString() ) throw ConfigTypeException(path) ;
    value = val.toString() ;
    return true ;
}

bool Config::value(const char *path, bool &value) const {
    Variant val ;
    if ( !container_.lookup(path, val) ) return false ;
    if ( !val.isBoolean() ) throw ConfigTypeException(path) ;
    value = val.toBoolean() ;
    return true ;
}

bool Config::value(const char *path, unsigned int &value) const {
    Variant val ;
    if ( !container_.lookup(path, val) ) return false ;
    if ( !val.isNumberInteger() ) throw ConfigTypeException(path) ;
    uint64_t v = val.toUnsignedInteger() ;
    if ( v > std::numeric_limits<unsigned int>::max() ) throw ConfigOverflowException(path) ;
    value = static_cast<unsigned int>(v) ;
    return true ;
}

bool Config::value(const char *path, int &value) const {
    Variant val ;
    if ( !container_.lookup(path, val) ) return false ;
    if ( !val.isNumberInteger() ) throw ConfigTypeException(path) ;
    int64_t v = val.toSignedInteger() ;
    if ( v > std::numeric_limits<int>::max() ) throw ConfigOverflowException(path) ;
    value = static_cast<int>(v) ;
    return true ;
}


bool Config::parseNameValue(ParseContext &ctx, Variant &v) {
    using namespace detail ;

    Token tk = ctx.tokenizer_->nextToken() ;

    if ( tk == TOKEN_IDENTIFIER ) {
        string name = ctx.tokenizer_->token_string_literal_ ;
        tk = ctx.tokenizer_->nextToken() ;
        if ( tk == TOKEN_EQUAL || tk == TOKEN_COLON ) {
            Variant val = parseValue(ctx) ;
            v[name] = val ;
            return true ;
        }
        else ctx.tokenizer_->throwException("unexpected token") ;

    } else if ( tk == TOKEN_EOF_DOCUMENT || tk == TOKEN_CLOSE_BRACE )
        return false ;
    else
        ctx.tokenizer_->throwException("unexpected token") ;

}

Variant Config::parseValue(Config::ParseContext &ctx) {
    using namespace detail ;

    Token tk = ctx.tokenizer_->nextToken() ;

    if ( tk == TOKEN_STRING ) {
        string res = ctx.tokenizer_->token_string_literal_ ;
        Token tk ;
        do {
            Token tk = ctx.tokenizer_->nextToken() ;
            if ( tk == TOKEN_STRING ) {
                res.append(ctx.tokenizer_->token_string_literal_) ;
            } else {
                ctx.tokenizer_->pushBack(tk);
            }
        } while ( tk == TOKEN_STRING ) ;
        return Variant(res) ;
    } else if ( tk == TOKEN_NUMBER ) {
        if ( ctx.tokenizer_->token_number_literal_.is_integer_ ) {
            return Variant(ctx.tokenizer_->token_number_literal_.toInt()) ;
        } else
            return Variant(ctx.tokenizer_->token_number_literal_.toDouble()) ;
    } else if ( tk == TOKEN_BOOLEAN ) {
        return Variant(ctx.tokenizer_->token_boolean_literal_) ;
    } else if ( tk == TOKEN_JSON_NULL ) {
        return Variant(nullptr) ;
    } else if ( tk == TOKEN_OPEN_BRACE ) {
        Variant v = Variant::Object() ;
        while ( parseNameValue(ctx, v) ) ;
        return v ;
    } else if ( tk == TOKEN_OPEN_BRACKET ) {
        Variant v = Variant::Array() ;
        Token tk = TOKEN_EMPTY ;
        do {
            Variant item = parseValue(ctx) ;
            v.append(item) ;
            tk = ctx.tokenizer_->nextToken() ;
            if ( tk == TOKEN_COMMA ) continue ;
            if ( tk != TOKEN_CLOSE_BRACKET )
                ctx.tokenizer_->throwException("unexpected token");

        } while ( tk != TOKEN_CLOSE_BRACKET ) ;
        return v ;
    } else if ( tk == TOKEN_INCLUDE ) {
        ifstream strm(ctx.tokenizer_->token_string_literal_) ;
        ParseContext ic ;
        ic.tokenizer_.reset(new JSONTokenizer(strm)) ;
        parse(ic, ctx.var_) ;
    }


}

void Config::parse(Config::ParseContext &ctx, Variant &v) {
    while( parseNameValue(ctx, v) ) ;

}


}
