#include <cvx/misc/json_reader.hpp>

#include <iterator>
#include <stack>
#include <cassert>
#include <ios>

#include "json_tokenizer.hpp"

using namespace std ;

namespace cvx {


JSONParseException::JSONParseException(const string &msg, uint line, uint col) {
    std::ostringstream strm ;
    strm << msg << ", at line " << line << ", column " << col ;
    msg_ = strm.str() ;
}

JSONReader::JSONReader(istream &strm)
{
    tokenizer_.reset(new detail::JSONTokenizer(strm)) ;
    current_token_ = JSONToken::UNDEFINED ;
    state_stack_.push(EMPTY_DOCUMENT) ;
}

void JSONReader::beginObject()
{
    auto t = peek() ;
    if ( t != JSONToken::BEGIN_OBJECT ) tokenizer_->throwException("expecting object") ;
    advance() ;
}

void JSONReader::endObject()
{
    auto t = peek() ;
    if ( t != JSONToken::END_OBJECT ) tokenizer_->throwException("unterminated object") ;
    advance() ;
}


void JSONReader::beginArray()
{
    auto t = peek() ;
    if ( t != JSONToken::BEGIN_ARRAY ) tokenizer_->throwException("expecting array") ;
    advance() ;
}

void JSONReader::endArray()
{
    auto t = peek() ;
    if ( t != JSONToken::END_ARRAY ) tokenizer_->throwException("unterminated array") ;
    advance() ;
}

void JSONReader::advance() {
    current_token_ = JSONToken::UNDEFINED ;
}

bool JSONReader::hasNext() {
    auto token = peek();
    return ( token != JSONToken::END_OBJECT && token != JSONToken::END_ARRAY );
}

bool JSONReader::nextBoolean()
{
     auto token = peek();
     advance() ;
     if ( token == JSONToken::BOOLEAN ) {
         return tokenizer_->token_boolean_literal_ ;
     } else
         tokenizer_->throwException("expecting boolean literal") ;

     return false ;
}

int64_t JSONReader::nextInt()
{
     auto token = peek();
     advance() ;
     if ( token == JSONToken::NUMBER) {
         return tokenizer_->token_number_literal_.toInt() ;
     } else
         tokenizer_->throwException("expecting number literal") ;

     return 0;
}

void JSONReader::nextNull()
{
    auto token = peek();
    advance() ;
    if ( token == JSONToken::JSON_NULL ) {

    } else
        tokenizer_->throwException("expecting null literal") ;

}

string JSONReader::nextString()
{
     auto token = peek();
     advance() ;
     if ( token == JSONToken::STRING) {
         return tokenizer_->token_string_literal_ ;
     } else
         tokenizer_->throwException("expecting string literal") ;

     return {};
}

double JSONReader::nextDouble()
{
     auto token = peek();
     advance() ;
     if ( token == JSONToken::NUMBER) {
         return tokenizer_->token_number_literal_.toDouble() ;
     } else
         tokenizer_->throwException("expecting number literal") ;

     return 0 ;
}

string JSONReader::nextName()  {
    auto token = peek();
    advance() ;
    if ( token != JSONToken::NAME )
        tokenizer_->throwException("expecting name") ;

    return tokenizer_->token_string_literal_ ;
}

void JSONReader::skipValue() {
    if ( !hasNext() || peek() == JSONToken::END_DOCUMENT ) {
        tokenizer_->throwException("no element left to skip") ;
    }

    int count = 0;
    skipping_ = true ;
    while (1) {
      auto token = peek() ;
      advance();
      if ( token == JSONToken::BEGIN_ARRAY || token == JSONToken::BEGIN_OBJECT) {
           count++;
      } else if ( token == JSONToken::END_ARRAY || token == JSONToken::END_OBJECT) {
           count--;
      }
      if ( count == 0 ) break ;
    }
    skipping_ = false ;
}

JSONToken JSONReader::nextValue(int tk) {
using namespace detail ;
    switch ( tk ) {
        case TOKEN_OPEN_BRACE:
            state_stack_.push(EMPTY_OBJECT) ;
            return JSONToken::BEGIN_OBJECT ;
        case TOKEN_OPEN_BRACKET:
            state_stack_.push(EMPTY_ARRAY) ;
            return JSONToken::BEGIN_ARRAY ;
        case TOKEN_JSON_NULL:
            return JSONToken::JSON_NULL ;
        case TOKEN_NUMBER:
            return JSONToken::NUMBER ;
        case TOKEN_BOOLEAN:
            return JSONToken::BOOLEAN ;
        case TOKEN_STRING:
            return JSONToken::STRING ;
    default:
        tokenizer_->throwException("unexpected token " + JSONTokenizer::token_to_string(tk)) ;
    }
}


JSONToken JSONReader::peek() {
    using namespace detail ;
    if ( current_token_ != JSONToken::UNDEFINED ) {
        return current_token_  ;
    }

    State state = state_stack_.top() ;

    if ( state == EMPTY_DOCUMENT ) {
        state_stack_.top() = NON_EMPTY_DOCUMENT ;

        auto tk = tokenizer_->nextToken() ;
        current_token_ = nextValue(tk) ;

        if ( current_token_ != JSONToken::BEGIN_ARRAY && current_token_ != JSONToken::BEGIN_OBJECT ) {
            tokenizer_->throwException("Expected JSON document to start with '[' or '{'");
        }

        return current_token_ ;

    } else if ( state == EMPTY_OBJECT || state == NON_EMPTY_OBJECT ) {
        auto tk = tokenizer_->nextToken() ;

        if ( state == EMPTY_OBJECT ) {
            if ( tk == TOKEN_CLOSE_BRACE ) {
                state_stack_.pop() ;
                return current_token_ = JSONToken::END_OBJECT ;
            }
        } else {
            if ( tk == TOKEN_CLOSE_BRACE ) {
                state_stack_.pop() ;
                return current_token_ = JSONToken::END_OBJECT ;
            } else if ( tk == TOKEN_COMMA ) {
                tk = tokenizer_->nextToken() ;
            } else {
                tokenizer_->throwException("unexpected token " + JSONTokenizer::token_to_string(tk)) ;
            }
        }

        state_stack_.top() = NON_EMPTY_OBJECT ;
        // read name

        if ( tk == TOKEN_STRING ) {
            state_stack_.push(DANGLING_NAME) ;
            return current_token_ = JSONToken::NAME ;
        } else
            tokenizer_->throwException("unexpected token " + JSONTokenizer::token_to_string(tk)) ;

    } else if ( state == EMPTY_ARRAY || state == NON_EMPTY_ARRAY ) {
        auto tk = tokenizer_->nextToken() ;

        if ( state == EMPTY_ARRAY ) {
            if ( tk == TOKEN_CLOSE_BRACKET ) {
                state_stack_.pop() ;
                return current_token_ = JSONToken::END_ARRAY ;
            }
        } else {
            if ( tk == TOKEN_CLOSE_BRACKET ) {
                state_stack_.pop() ;
                return current_token_ = JSONToken::END_ARRAY ;
            } else if ( tk == TOKEN_COMMA ) {
                tk = tokenizer_->nextToken() ;
            } else {
                tokenizer_->throwException("unexpected token " + JSONTokenizer::token_to_string(tk)) ;
            }
        }

        state_stack_.top() = NON_EMPTY_ARRAY ;
        // read value

        return current_token_ = nextValue(tk) ;
    } else if ( state == DANGLING_NAME ) {
        auto tk = tokenizer_->nextToken() ;

        if ( tk == TOKEN_COLON ) {
            state_stack_.pop() ;
            tk = tokenizer_->nextToken() ;
            return current_token_ = nextValue(tk) ;
        } else {
            tokenizer_->throwException("unexpected token " + JSONTokenizer::token_to_string(tk)) ;
        }
    } else if ( state == NON_EMPTY_DOCUMENT ) {
        state_stack_.pop() ;
        auto tk = tokenizer_->nextToken() ;

        if ( tk != TOKEN_EOF_DOCUMENT )
            tokenizer_->throwException("unexpected token " + JSONTokenizer::token_to_string(tk)) ;

        return current_token_ = JSONToken::END_DOCUMENT ;
    }

    assert(true) ;
    return current_token_ = JSONToken::UNDEFINED ; // should not end up here
}


}
