#include <cvx/misc/config.hpp>

#include "json_tokenizer.hpp"

#include <iostream>
#include <fstream>

using namespace std ;

namespace cvx {

Config::Config(istream &strm) {
    using namespace detail ;
    ParseContext ctx ;
    ctx.tokenizer_.reset(new JSONTokenizer(strm, true)) ;
    container_ = Variant::Object() ;
    parse(ctx, container_) ;
}

Config::~Config(){

}

void Config::parseNameValue(ParseContext &ctx, Variant &v) {
    using namespace detail ;

    Token tk = ctx.tokenizer_->nextToken() ;
    cout << JSONTokenizer::token_to_string(tk)  << endl ;
    if ( tk == TOKEN_IDENTIFIER ) {
        string name = ctx.tokenizer_->token_string_literal_ ;
        tk = ctx.tokenizer_->nextToken() ;
        if ( tk == TOKEN_EQUAL || tk == TOKEN_COLON ) {
            Variant val = parseValue(ctx) ;
            v[name] = val ;
        }
        else ctx.tokenizer_->throwException("unexpected token") ;

    }
}

Variant Config::parseValue(Config::ParseContext &ctx) {
    using namespace detail ;

    Token tk = ctx.tokenizer_->nextToken() ;

    if ( tk == TOKEN_STRING ) {
        return Variant(ctx.tokenizer_->token_string_literal_) ;
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
        while ( tk != TOKEN_CLOSE_BRACE ) {
            parseNameValue(ctx, v) ;
        }
        return v ;
    } else if ( tk == TOKEN_OPEN_BRACKET ) {
        Variant v = Variant::Array() ;
        while ( tk != TOKEN_CLOSE_BRACKET ) {
            Variant item = parseValue(ctx) ;
            v.append(item) ;
        }
        return v ;
    } else if ( tk == TOKEN_INCLUDE ) {
        ifstream strm(ctx.tokenizer_->token_string_literal_) ;
        ParseContext ic ;
        ic.tokenizer_.reset(new JSONTokenizer(strm)) ;
        parse(ic, ctx.var_) ;
    }


}

void Config::parse(Config::ParseContext &ctx, Variant &v)
{
    using namespace detail ;
    Token tk = TOKEN_EMPTY ;
    while( tk != TOKEN_EOF_DOCUMENT ) {
        parseNameValue(ctx, v) ;
    }

}


}
