#ifndef CVX_UTIL_VARIANT_HPP
#define CVX_UTIL_VARIANT_HPP

#include <cstdint>
#include <map>
#include <vector>
#include <iomanip>
#include <iostream>
#include <functional>
#include <sstream>
#include <string>
#include <cassert>

#include <cvx/misc/strings.hpp>
#include <cvx/misc/json_reader.hpp>

// very lighweight variant class e.g. to enable json object parsing
//
// e.g.  Variant v(Variant::Object{
//                              {"name", 3},
//                              {"values", Variant::Array{ {2,  "s" } } }
//                }) ;
//       cout << v.toJSON() << endl ;

namespace cvx {
class Variant {

public:

    using Object = std::map<std::string, Variant> ;
    using Array = std::vector<Variant> ;
    using Function = std::function<Variant(const Variant &)> ;

    using signed_integer_t = int64_t ;
    using unsigned_integer_t = uint64_t ;
    using float_t = double ;
    using string_t = std::string ;
    using boolean_t = bool ;

    using Dictionary = std::map<std::string, std::string> ;

    enum class Type : uint8_t {
        Undefined, Null,  Object, Array, String, Boolean, UnsignedInteger, SignedInteger, Float, Function
    };

    // constructors

    Variant(): tag_(Type::Undefined) {}

    Variant(std::nullptr_t): tag_(Type::Null) {}
    Variant(boolean_t v) noexcept : tag_(Type::Boolean) { data_.b_ = v ; }

    Variant(int v) noexcept: Variant((signed_integer_t)v) {}
    Variant(unsigned int v) noexcept: Variant((unsigned_integer_t)v) {}

    Variant(int64_t v) noexcept: tag_(Type::SignedInteger) { data_.i_ = v ; }
    Variant(uint64_t v) noexcept: tag_(Type::UnsignedInteger) { data_.u_ = v ; }

    Variant(Function v): tag_(Type::Function) { new (&data_.f_) Function(v) ; }

    Variant(float_t v) noexcept: tag_(Type::Float) { data_.d_ = v ; }

    Variant(const char *value) {
        tag_ = Type::String ;
        new (&data_.s_) string_t(value) ;
    }

    Variant(const string_t& value) {
        tag_ = Type::String ;
        new (&data_.s_) string_t(value) ;
    }

    Variant(const char value) {
        tag_ = Type::String ;
        new (&data_.s_) string_t(1, value) ;
    }

    Variant(string_t&& value)  {
        tag_ = Type::String ;
        new (&data_.s_) string_t(value) ;
    }


    Variant(const Object& value): tag_(Type::Object) {
        new (&data_.o_) Object(value) ;
    }

    Variant(Object&& value): tag_(Type::Object) {
        new (&data_.o_) Object(std::move(value)) ;
    }

    Variant(const Array& value): tag_(Type::Array) {
        new (&data_.a_) Array(value) ;
    }

    Variant(Array&& value): tag_(Type::Array) {
        new (&data_.a_) Array(std::move(value)) ;
    }

    Variant(std::initializer_list<Variant> values, bool auto_type = true, Type t = Type::Array) {
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

    static Variant array(std::initializer_list<Variant> values) {
        return Variant(values, false, Type::Array) ;
    }

    static Variant object(std::initializer_list<Variant> values) {
        return Variant(values, false, Type::Object) ;
    }


    ~Variant() {
        destroy() ;
    }

    Variant(const Variant& other) {
        create(other) ;
    }

    Variant &operator=(const Variant &other) {
        if ( this != &other ) {
            destroy() ;
            create(other) ;
        }
        return *this ;
    }

    Variant(Variant&& other): tag_(other.tag_) {
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

    // make Object from a dictionary
    static Variant fromDictionary(const Dictionary &dict) {
        Variant::Object obj ;
        for( const auto &p: dict )
            obj.insert({p.first, p.second}) ;
        return std::move(obj) ;
    }

    // make Array from dictionary where each element is the Object {<keyname>: <key>, <valname>: <val>}
    static Variant fromDictionaryAsArray(const Dictionary &dict, const std::string &keyname = "key", const std::string &valname = "val") {
        Variant::Array ar ;
        for( const auto &p: dict )
            ar.emplace_back(Variant::Object({{keyname, p.first}, {valname, p.second}})) ;
        return std::move(ar) ;
    }

    // make Array from a vector of values
    template<class T>
    static Variant fromVector(const std::vector<T> &vals) {
        Variant::Array ar ;
        for( const auto &p: vals )
            ar.push_back(p) ;
        return std::move(ar) ;
    }

    void append(const std::string &key, const Variant &val) {
        if ( isObject() )
            data_.o_.insert({key, val}) ;
    }

    void append(const Variant &val) {
        if ( isArray() )
            data_.a_.push_back(val) ;
    }

    explicit operator bool() const noexcept {
        if ( isBoolean() ) {
            return data_.b_ ;
        } else if ( isUndefined() || isNull() ) {
            return false ;
        }
        return true ;
    }

    // check object type

    bool isObject() const { return tag_ == Type::Object ; }
    bool isArray() const { return tag_ == Type::Array ; }
    bool isNull() const { return tag_ == Type::Null ; }
    bool isUndefined() const { return tag_ == Type::Undefined ; }
    bool isBoolean() const { return tag_ == Type::Boolean ; }
    bool isString() const { return tag_ == Type::String; }

    bool isNumber() const {
        return ( tag_ == Type::UnsignedInteger ) ||
                ( tag_ == Type::SignedInteger ) ||
                ( tag_ == Type::Float )
                ;
    }

    bool isNumberInteger() const {
        return ( tag_ == Type::UnsignedInteger ) ||
                ( tag_ == Type::SignedInteger ) ;
    }

    bool isNumberFloat() const {
        return ( tag_ == Type::Float ) ;
    }

    // check if variant stores simple type string, number, integer or boolean
    bool isPrimitive() const {
        return ( tag_ == Type::String ||
                 tag_ == Type::UnsignedInteger ||
                 tag_ == Type::SignedInteger ||
                 tag_ == Type::Float ||
                 tag_ == Type::Boolean
                 ) ;
    }

    // convert value to string
    std::string toString() const {
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

    double toFloat() const {
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

    signed_integer_t toSignedInteger() const {
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

    unsigned_integer_t toUnsignedInteger() const {
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

    bool toBoolean() const {
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

    Object toObject() const {
        switch (tag_)
        {
        case Type::Object:
            return data_.o_ ;

        default:
            return Object();
        }
    }

    // Return the keys of an Object otherwise an empty list
    std::vector<std::string> keys() const {
        std::vector<std::string> res ;

        if ( !isObject() ) return res ;

        for ( const auto &p: data_.o_ )
            res.push_back(p.first) ;
        return res ;
    }

    // length of object or array or string, zero otherwise
    size_t length() const {
        if ( isObject() )
            return data_.o_.size() ;
        else if ( isArray() ) {
            return data_.a_.size() ;
        } else if ( tag_ == Type::String )
            return data_.s_.length() ;
        else return 0 ;
    }

    // Returns a member value given the key. The key is of the form <member1>[.<member2>. ... <memberN>]
    // If this is not an object or the key is not found it returns false

    bool lookup(const std::string &key, Variant &val) const {
        if ( key.empty() ) return false ;

        const Variant *current = this ;
        if ( !current->isObject() ) return false ;

        size_t start = 0, end = 0;

        while ( end != std::string::npos) {
            end = key.find('.', start) ;
            std::string subkey = key.substr(start, end == std::string::npos ? std::string::npos : end - start) ;

            try {
                val = current->fetchKey(subkey) ;

                if ( end != std::string::npos ) {
                    current = &val ;
                    start = end+1 ;
                }
                else {
                    return true ;
                }

            } catch ( std::exception & ) {
                return false ;
            }

        }

        return false ;

    }

    // get value for key path and return default value if not found
    Variant value(const std::string &key, const Variant &defaultValue) const {
        Variant val ;
        if ( !lookup(key, val) ) return defaultValue ;
        else return val ;
    }

    const Variant &at(const std::string &key) const {
        return fetchKey(key) ;

    }

    // same as above but it returns a non-const reference or throws exception if not found or item is not an object
    Variant &at(const std::string &key)  {
        return fetchKey(key) ;
    }



    // return an element of an array
    const Variant &at(uint idx) const { return fetchIndex(idx) ; }

    // overloaded indexing operators
    const Variant &operator [] (const std::string &key) const {
        return fetchKey(key) ;
    }

    Variant &operator [] (const std::string &key) {
        return fetchKey(key, false) ;
    }

    const Variant &operator [] (uint idx) const {
        return fetchIndex(idx) ;
    }

    Variant &operator [] (uint idx) {
        return fetchIndex(idx, false) ;
    }

    Type type() const { return tag_ ; }

    // JSON encoder
    void toJSON(std::ostream &strm) const {

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

    // export as JSON string

    std::string toJSON() const {
        std::ostringstream strm ;
        toJSON(strm) ;
        return strm.str() ;
    }


    // import from JSON stream

    static Variant fromJSON(std::istream &strm) {
        JSONReader reader(strm) ;

        try {
            return parseJSONValue(reader) ;
        }
        catch ( cvx::JSONParseException &e ) {
            std::cerr << e.what() << ")" ;
            return Variant() ;
        }
    }

    static Variant fromJSON(const std::string &src) {
        std::istringstream strm(src) ;
        return fromJSON(strm) ;
    }

    // iterates dictionaries or arrays

    class iterator {
    public:
        iterator(const Variant &obj, bool set_to_begin = false): obj_(obj) {
            if ( obj.isObject() ) o_it_ = ( set_to_begin ) ? obj_.data_.o_.begin() : obj_.data_.o_.end();
            else if ( obj.isArray() ) a_it_ = ( set_to_begin ) ? obj_.data_.a_.begin() : obj_.data_.a_.end();
        }

        const Variant & operator*() const {
            if ( obj_.isObject() ) {
                assert( o_it_ != obj_.data_.o_.end() ) ;
                return o_it_->second ;
            }
            else if ( obj_.isArray() ) {
                assert( a_it_ != obj_.data_.a_.end() ) ;
                return *a_it_ ;
            }
            else {

                return Variant::undefined() ;
            }
        }

        const Variant *operator->() const {
            if ( obj_.isObject() ) {
                assert( o_it_ != obj_.data_.o_.end() ) ;
                return &(o_it_->second) ;
            }
            else if ( obj_.isArray() ) {
                assert( a_it_ != obj_.data_.a_.end() ) ;
                return &(*a_it_) ;
            }
            else {
                return &Variant::undefined() ;
            }
        }

        iterator const operator++(int) {
            iterator it = *this;
            ++(*this);
            return it;
        }


        iterator& operator++() {
            if ( obj_.isObject() ) ++o_it_ ;
            else if ( obj_.isArray() ) ++a_it_ ;

            return *this ;
        }

        iterator const operator--(int) {
            iterator it = *this;
            --(*this);
            return it;
        }


        iterator& operator--() {
            if ( obj_.isObject() ) --o_it_ ;
            else if ( obj_.isArray() ) --a_it_ ;

            return *this ;
        }

        bool operator==(const iterator& other) const
        {
            assert(&obj_ == &other.obj_) ;

            if ( obj_.isObject() ) return (o_it_ == other.o_it_) ;
            else if ( obj_.isArray() ) return (a_it_ == other.a_it_) ;
            else return true ;
        }

        bool operator!=(const iterator& other) const {
            return ! operator==(other);
        }

        std::string key() const {
            if ( obj_.isObject() ) return o_it_->first ;
            else return std::string() ;
        }

        const Variant &value() const {
            return operator*();
        }

    private:
        const Variant &obj_ ;
        Object::const_iterator o_it_ ;
        Array::const_iterator a_it_ ;
    } ;

    iterator begin() const {
        return iterator(*this, true) ;
    }

    iterator end() const {
        return iterator(*this, false) ;
    }

    static const Variant &null() {
        static Variant null_value ;
        return null_value ;
    }

    static const Variant &undefined() {
        static Variant undefined_value ;
        undefined_value.tag_ = Type::Undefined ;
        return undefined_value ;
    }

    Variant invoke(const Variant &args) {
        if ( tag_ != Type::Function ) return undefined() ;
        else return (data_.f_)(args) ;
    }

private:

    static Variant parseJSONValue(JSONReader &reader) {

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

    static Variant parseJSONObject(JSONReader &reader) {

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

    static Variant parseJSONArray(JSONReader &reader) {

        Variant::Array result ;

        reader.beginArray() ;

        while ( reader.hasNext() ) {
            result.emplace_back(parseJSONValue(reader)) ;
        }

        reader.endArray() ;
        return result ;

    }

    // Original: https://gist.github.com/kevinkreiser/bee394c60c615e0acdad

    static std::string json_escape_string(const std::string &str) {
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


    const Variant &fetchKey(const std::string &key) const {
            if (!isObject() ) throw std::runtime_error("Trying to index an object which is not dictionary") ;

            auto it = data_.o_.find(key) ;
            if ( it == data_.o_.end() ) {
                std::ostringstream strm ;
                strm << "key '" << key << "' not found" ;
                throw std::out_of_range( strm.str());
            }
            else return it->second ; // return reference
    }

    Variant &fetchKey(const std::string &key, bool safe = true ) {
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

    const Variant &fetchIndex(uint idx) const {
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

    Variant &fetchIndex(uint idx, bool safe = true)  {
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

    void destroy() {
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

    void create(const Variant &other) {
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

private:

    union Data {
        Object    o_;
        Array     a_ ;
        string_t s_ ;
        boolean_t   b_ ;
        unsigned_integer_t u_ ;
        signed_integer_t   i_ ;
        float_t     d_ ;
        Function f_ ;

        Data() {}
        ~Data() {}
    } ;

    Data data_ ;
    Type tag_ ;

};

}

#endif

