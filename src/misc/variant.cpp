#include <cvx/misc/variant.hpp>
#include "config.hpp"

namespace cvx {

Variant::Variant(std::initializer_list<Variant> values, bool auto_type, Variant::Type t) {
    bool is_an_object = std::all_of(values.begin(), values.end(),
                                    [](const Variant& element) {
        return element.isArray() && element.length() == 2 && element[0].isString() ;
    });

    if ( !auto_type ) {
        if ( t == Type::Array ) is_an_object = false ;
        else if ( t == Type::Object && !is_an_object)
            throw std::runtime_error("cannot create object from initializer list");
    }

    if (is_an_object) {
        Object result ;

        std::for_each(values.begin(), values.end(), [&](const Variant& element)
        {
            result.emplace(element.data_.a_[0].data_.s_,
                    element.data_.a_[1]) ;
        });
        tag_ = Type::Object ;
        new (&data_.o_) Object(result) ;
    }
    else {
        tag_ = Type::Array ;
        new (&data_.o_) Array(values) ;
    }

}

Variant::Variant(Variant &&other): tag_(other.tag_) {
    switch (tag_)
    {
    case Type::Object:
        new (&data_.o_) Object(std::move(other.data_.o_)) ;
        break;
    case Type::Array:
        new (&data_.a_) Array(std::move(other.data_.a_)) ;
        break;
    case Type::String:
        new (&data_.s_) string_t(std::move(other.data_.s_)) ;
        break;
    case Type::Boolean:
        data_.b_ = other.data_.b_ ;
        break;
    case Type::UnsignedInteger:
        data_.u_ = other.data_.u_ ;
        break;
    case Type::SignedInteger:
        data_.i_ = other.data_.i_ ;
        break;
    case Type::Float:
        data_.d_ = other.data_.d_ ;
        break;
    case Type::Function:
        new (&data_.f_) Function(std::move(other.data_.f_)) ;
    default:
        break;
    }

    other.tag_ = Type::Undefined ;
}

Variant Variant::fromDictionary(const Variant::Dictionary &dict) {
    Variant::Object obj ;
    for( const auto &p: dict )
        obj.insert({p.first, p.second}) ;
    return std::move(obj) ;
}

Variant Variant::fromDictionaryAsArray(const Variant::Dictionary &dict, const std::string &keyname, const std::string &valname) {
    Variant::Array ar ;
    for( const auto &p: dict )
        ar.emplace_back(Variant::Object({{keyname, p.first}, {valname, p.second}})) ;
    return std::move(ar) ;
}

std::string Variant::toString() const {
    switch (tag_)
    {
    case Type::String:
        return data_.s_;
    case Type::Boolean: {
        std::ostringstream strm ;
        strm << data_.b_ ;
        return strm.str() ;
    }
    case Type::UnsignedInteger:
        return std::to_string(data_.u_) ;
    case Type::SignedInteger:
        return std::to_string(data_.i_) ;
    case Type::Float:
        return std::to_string(data_.d_) ;
    default:
        return std::string();
    }
}

double Variant::toFloat() const {
    switch (tag_)
    {
    case Type::String:
        try {
        return std::stod(data_.s_);
    }
        catch ( ... ) {
        return 0.0 ;
    }

    case Type::Boolean:
        return (double)data_.b_ ;
    case Type::UnsignedInteger:
        return (double)data_.u_ ;
    case Type::SignedInteger:
        return (double)data_.i_ ;
    case Type::Float:
        return (double)data_.d_ ;
    default:
        return 0.0;
    }
}

Variant::signed_integer_t Variant::toSignedInteger() const {
    switch (tag_)
    {
    case Type::String:
        try {
        return std::stoll(data_.s_);
    }
        catch ( ... ) {
        return 0 ;
    }
    case Type::Boolean:
        return static_cast<signed_integer_t>(data_.b_) ;
    case Type::UnsignedInteger:
        return static_cast<signed_integer_t>(data_.u_) ;
    case Type::SignedInteger:
        return static_cast<signed_integer_t>(data_.i_) ;
    case Type::Float:
        return static_cast<signed_integer_t>(data_.d_) ;
    default:
        return 0;
    }
}

Variant::unsigned_integer_t Variant::toUnsignedInteger() const {
    switch (tag_)
    {
    case Type::String:
        try {
        return std::stoull(data_.s_);
    }
        catch ( ... ) {
        return 0 ;
    }
    case Type::Boolean:
        return static_cast<unsigned_integer_t>(data_.b_) ;
    case Type::UnsignedInteger:
        return static_cast<unsigned_integer_t>(data_.u_) ;
    case Type::SignedInteger:
        return static_cast<unsigned_integer_t>(data_.i_) ;
    case Type::Float:
        return static_cast<unsigned_integer_t>(data_.d_) ;
    default:
        return 0;
    }
}

bool Variant::toBoolean() const {
    switch (tag_)
    {
    case Type::String:
        return !(data_.s_.empty()) ;
    case Type::Boolean:
        return data_.b_ ;
    case Type::UnsignedInteger:
        return static_cast<bool>(data_.u_) ;
    case Type::SignedInteger:
        return static_cast<bool>(data_.u_) ;
    case Type::Float:
        return static_cast<bool>(data_.d_ != 0.0) ;
    default:
        return false;
    }
}

Variant::Object Variant::toObject() const {
    switch (tag_)
    {
    case Type::Object:
        return data_.o_ ;

    default:
        return Object();
    }
}

std::vector<std::string> Variant::keys() const {
    std::vector<std::string> res ;

    if ( !isObject() ) return res ;

    for ( const auto &p: data_.o_ )
        res.push_back(p.first) ;
    return res ;
}

size_t Variant::length() const {
    if ( isObject() )
        return data_.o_.size() ;
    else if ( isArray() ) {
        return data_.a_.size() ;
    } else if ( tag_ == Type::String )
        return data_.s_.length() ;
    else return 0 ;
}

bool Variant::lookup(const std::string &key, Variant &val) const {
    if ( key.empty() ) return false ;

    const Variant *current = this ;
    if ( !current->isObject() ) return false ;

    size_t start = 0, end = 0;

    while ( end != std::string::npos) {
        end = key.find('.', start) ;
        std::string subkey = key.substr(start, end == std::string::npos ? std::string::npos : end - start) ;

        try {
            const Variant &v =
                    (*current).fetchConstKey(subkey) ;

            if ( end != std::string::npos ) {
                current = &v ;
                start = end+1 ;
            }
            else {
                val = v ;
                return true ;
            }

        } catch ( std::exception & ) {
            return false ;
        }

    }

    return false ;

}

void Variant::toJSON(std::ostream &strm) const {

    switch ( tag_ ) {
    case Type::Object: {
        strm << "{" ;
        auto it = data_.o_.cbegin() ;
        if ( it != data_.o_.cend() ) {
            strm << json_escape_string(it->first) << ": " ;
            it->second.toJSON(strm) ;
            ++it ;
        }
        while ( it != data_.o_.cend() ) {
            strm << ", " ;
            strm << json_escape_string(it->first) << ": " ;
            it->second.toJSON(strm) ;
            ++it ;
        }
        strm << "}" ;
        break ;
    }
    case Type::Array: {
        strm << "[" ;
        auto it = data_.a_.cbegin() ;
        if ( it != data_.a_.cend() ) {
            it->toJSON(strm) ;
            ++it ;
        }
        while ( it != data_.a_.cend() ) {
            strm << ", " ;
            it->toJSON(strm) ;
            ++it ;
        }

        strm << "]" ;
        break ;
    }
    case Type::String:
    {
        strm << json_escape_string(data_.s_) ;
        break ;
    }
    case Type::Boolean: {
        strm << ( data_.b_ ? "true" : "false") ;
        break ;
    }
    case Type::Null: {
        strm << "null" ;
        break ;
    }
    case Type::Float: {
        strm << data_.d_ ;
        break ;
    }
    case Type::UnsignedInteger: {
        strm << data_.u_ ;
        break ;
    }
    case Type::SignedInteger: {
        strm << data_.i_ ;
        break ;
    }
    }
}

Variant Variant::fromJSON(std::istream &strm) {
    JSONReader reader(strm) ;

    try {
        return parseJSONValue(reader) ;
    }
    catch ( cvx::JSONParseException &e ) {
        std::cerr << e.what() << ")" ;
        return Variant() ;
    }
}

Variant Variant::fromConfigFile(const std::string &path, const std::string &inc_path) {
    try {
        return ConfigParser::loadFile(path, inc_path) ;
    } catch ( ConfigParseException &e ) {
        throw std::runtime_error(e.what()) ;
    }
}

Variant Variant::fromConfigString(const std::string &src, const std::string &inc_path) {
    try {
        return ConfigParser::loadString(src, inc_path) ;
    } catch ( ConfigParseException &e ) {
        throw std::runtime_error(e.what()) ;
    }
}

Variant Variant::parseJSONValue(JSONReader &reader) {

    JSONToken tk = reader.peek() ;
    if ( tk == JSONToken::BEGIN_OBJECT ) {
        return parseJSONObject(reader) ;
    }
    else if ( tk == JSONToken::BEGIN_ARRAY ) {
        return parseJSONArray(reader) ;
    }  else if ( tk == JSONToken::STRING ) {
        return Variant(reader.nextString()) ;

    } else if ( tk == JSONToken::BOOLEAN ) {
        return Variant(reader.nextBoolean()) ;
    } else if ( tk == JSONToken::JSON_NULL ) {
        return Variant::null() ;
    } else if ( tk == JSONToken::NUMBER ) {
        return Variant(reader.nextDouble()) ;
    } else return nullptr ;

}

Variant Variant::parseJSONObject(JSONReader &reader) {

    Variant::Object result ;

    reader.beginObject() ;
    std::string key ;
    while ( reader.hasNext() ) {
        key = reader.nextName() ;
        Variant value = parseJSONValue(reader) ;
        result.emplace(key, value) ;
    }
    reader.endObject() ;

    return result ;

}

Variant Variant::parseJSONArray(JSONReader &reader) {

    Variant::Array result ;

    reader.beginArray() ;

    while ( reader.hasNext() ) {
        result.emplace_back(parseJSONValue(reader)) ;
    }

    reader.endArray() ;
    return result ;

}

// Original: https://gist.github.com/kevinkreiser/bee394c60c615e0acdad


std::string Variant::json_escape_string(const std::string &str) {
    std::stringstream strm ;
    strm << '"';

    for (const auto& c : str) {
        switch (c) {
        case '\\': strm << "\\\\"; break;
        case '"': strm << "\\\""; break;
        case '/': strm << "\\/"; break;
        case '\b': strm << "\\b"; break;
        case '\f': strm << "\\f"; break;
        case '\n': strm << "\\n"; break;
        case '\r': strm << "\\r"; break;
        case '\t': strm << "\\t"; break;
        default:
            if(c >= 0 && c < 32) {
                //format changes for json hex
                strm.setf(std::ios::hex, std::ios::basefield);
                strm.setf(std::ios::uppercase);
                strm.fill('0');
                //output hex
                strm << "\\u" << std::setw(4) << static_cast<int>(c);
            }
            else
                strm << c;
            break;
        }
    }
    strm << '"';

    return strm.str() ;
}

const Variant &Variant::fetchConstKey(const std::string &key) const {
    if (!isObject() ) throw std::runtime_error("Trying to index an object which is not dictionary") ;

    auto it = data_.o_.find(key) ;
    if ( it == data_.o_.end() ) {
        std::ostringstream strm ;
        strm << "key '" << key << "' not found" ;
        throw std::out_of_range( strm.str());
    }
    else return it->second ; // return reference
}

Variant &Variant::fetchKey(const std::string &key, bool safe) {
    if ( safe ) {
        if (!isObject() ) throw std::runtime_error("Trying to index an object which is not dictionary") ;

        auto it = data_.o_.find(key) ;
        if ( it == data_.o_.end() ) {
            std::ostringstream strm ;
            strm << "key '" << key << "' not found" ;
            throw std::out_of_range( strm.str());
        }
        else return it->second ; // return reference
    } else {
        if ( isNull() || isUndefined() ) {
            *this = std::move(Object());
            return data_.o_[key] ;
        } else if ( isObject() ) {
            return data_.o_[key] ;
        }
        else throw std::runtime_error("Trying to index an object which is not dictionary") ;
    }
}

const Variant &Variant::fetchIndex(uint idx) const {
    if (!isArray()) throw std::runtime_error("Trying to index an object which is not array") ;

    if ( idx < data_.a_.size() ) {
        const Variant &v = (data_.a_)[idx] ;
        return v ; // reference to item
    }
    else {
        std::ostringstream strm ;
        strm << "index '" <<idx << "' not in array of size " << data_.a_.size() ;
        throw std::out_of_range( strm.str() ) ;
    }
}

Variant &Variant::fetchIndex(uint idx, bool safe)  {
    if ( safe ) {
        if (!isArray()) throw std::runtime_error("Trying to index an object which is not array") ;

        if ( idx < data_.a_.size() ) {
            Variant &v = (data_.a_)[idx] ;
            return v ; // reference to item
        }
        else {
            std::ostringstream strm ;
            strm << "index '" <<idx << "' not in array of size " << data_.a_.size() ;
            throw std::out_of_range( strm.str() ) ;
        }
    } else {
        if ( isNull() || isUndefined() ) {
            *this = std::move(Array()) ;
            data_.a_.insert(data_.a_.end(), idx - data_.a_.size()+1, Variant()) ;
            return  (data_.a_)[idx] ;
        } else if ( isArray() ) {
            if ( idx < data_.a_.size() ) {
                Variant &v = (data_.a_)[idx] ;
                return v ; // reference to item
            } else {
                data_.a_.insert(data_.a_.end(), idx - data_.a_.size()+1, Variant()) ;
                return  (data_.a_)[idx] ;
            }
        }
        else
            throw std::runtime_error("Trying to index an object which is not dictionary") ;
    }
}

void Variant::destroy() {
    switch (tag_) {
    case Type::Object:
        data_.o_.~Object() ;
        break ;
    case Type::Array:
        data_.a_.~Array() ;
        break ;
    case Type::String:
        data_.s_.~string_t() ;
        break ;
    case Type::Function:
        data_.f_.~Function() ;
        break ;
    }
}

void Variant::create(const Variant &other) {
    tag_ = other.tag_ ;
    switch (tag_)
    {
    case Type::Object:
        new ( &data_.o_ ) Object(other.data_.o_) ;
        break;
    case Type::Array:
        new ( &data_.a_ ) Array(other.data_.a_) ;
        break;
    case Type::String:
        new ( &data_.s_ ) string_t(other.data_.s_) ;
        break;
    case Type::Function:
        new ( &data_.f_ ) Function(other.data_.f_) ;
        break;
    case Type::Boolean:
        data_.b_ = other.data_.b_ ;
        break;
    case Type::UnsignedInteger:
        data_.u_ = other.data_.u_ ;
        break;
    case Type::SignedInteger:
        data_.i_ = other.data_.i_ ;
        break;
    case Type::Float:
        data_.d_ = other.data_.d_ ;
        break;
    default:
        break;
    }

}







}
