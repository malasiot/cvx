#include "config.hpp"

#include "json_tokenizer.hpp"

#include <iostream>
#include <fstream>

#include <cvx/misc/path.hpp>

using namespace std ;

namespace cvx {


ConfigParser::~ConfigParser(){
}

Variant ConfigParser::loadFile(const std::string &fpath, const std::string &inc_path) {
    using namespace detail ;

    ifstream strm(fpath) ;
    if ( !strm ) throw ConfigParseException("Cannot load config file: " + fpath) ;

    Variant container ;

    ParseContext ctx ;

    ctx.tokenizer_.reset(new JSONTokenizer(strm, true)) ;
    ctx.inc_path_ = inc_path ;
    ctx.path_ = fpath ;

    try {
        parse(ctx, container) ;
        return container ;
    } catch ( JSONTokenizerParseException &e ) {
        string msg = "Error while parsing: " + fpath + "\n" ;
        msg += e.what() ;
        throw ConfigParseException(msg) ;
    }
}

Variant ConfigParser::loadString(const std::string &src, const std::string &inc_path) {
    using namespace detail ;

    istringstream strm(src) ;

    Variant container ;

    ParseContext ctx ;

    ctx.tokenizer_.reset(new JSONTokenizer(strm, true)) ;
    ctx.inc_path_ = inc_path ;

    try {
        parse(ctx, container) ;
        return container ;
    } catch ( JSONTokenizerParseException &e ) {
        string msg = "Error while parsing from string\n" ;
        msg += e.what() ;
        throw ConfigParseException(msg) ;
    }
}


bool ConfigParser::parseNameValue(ParseContext &ctx, Variant &v) {
    using namespace detail ;

    Token tk = ctx.tokenizer_->nextToken() ;
    if ( tk == TOKEN_COMMA )
        tk = ctx.tokenizer_->nextToken() ;

    if ( tk == TOKEN_IDENTIFIER ) {
        string name = ctx.tokenizer_->token_string_literal_ ;
        tk = ctx.tokenizer_->nextToken() ;

        if ( tk == TOKEN_EQUAL || tk == TOKEN_COLON ) {
            Variant val = parseValue(ctx) ;
            v[name] = val ;

            return true ;
        }
        else ctx.tokenizer_->throwException("unexpected token") ;

    } else if ( tk == TOKEN_INCLUDE ) {
        string ipath = ctx.tokenizer_->token_string_literal_ ;

        cvx::Path p(ipath) ;

        string apath = p.absolute(ctx.inc_path_.empty() ? Path::currentWorkingDir(): ctx.inc_path_) ;
        ifstream strm(apath) ;

        if ( !strm ) throw ConfigParseException("Cannot load include file: " + apath) ;

        ParseContext ic ;
        ic.tokenizer_.reset(new JSONTokenizer(strm, true)) ;
        ic.inc_path_ = ctx.inc_path_ ;
        ic.path_ = apath ;

        try {
            parse(ic, v) ;
        } catch ( JSONTokenizerParseException &e ) {
            string msg = "Error while parsing: " + ic.path_ + "\n" ;
            msg += e.what() ;
            throw ConfigParseException(msg) ;
        }

    } else if ( tk == TOKEN_EOF_DOCUMENT || tk == TOKEN_CLOSE_BRACE )
        return false ;
    else
        ctx.tokenizer_->throwException("unexpected token") ;

}

Variant ConfigParser::parseValue(ConfigParser::ParseContext &ctx) {
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
        ctx.skip_comma_ = false ;
        do {
            Variant item = parseValue(ctx) ;
            v.append(item) ;
            tk = ctx.tokenizer_->nextToken() ;
            if ( tk == TOKEN_COMMA ) continue ;
            else if ( tk == TOKEN_CLOSE_BRACKET ) break ;
            else ctx.tokenizer_->pushBack(tk) ;

        } while ( tk != TOKEN_CLOSE_BRACKET ) ;
        ctx.skip_comma_ = true ;
        return v ;
    }


}

void ConfigParser::parse(ConfigParser::ParseContext &ctx, Variant &v) {
    while( parseNameValue(ctx, v) ) ;

}


}
