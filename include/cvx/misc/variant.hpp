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
#include <type_traits>

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

    Variant(std::initializer_list<Variant> values, bool auto_type = true, Type t = Type::Array);

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

    Variant(Variant&& other);

    // make Object from a dictionary
    static Variant fromDictionary(const Dictionary &dict);

    // make Array from dictionary where each element is the Object {<keyname>: <key>, <valname>: <val>}
    static Variant fromDictionaryAsArray(const Dictionary &dict, const std::string &keyname = "key", const std::string &valname = "val");

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
    std::string toString() const;

    double toFloat() const;

    signed_integer_t toSignedInteger() const;

    unsigned_integer_t toUnsignedInteger() const;

    bool toBoolean() const;

    Object toObject() const;

    // get primitive value

    template<typename T>
    std::enable_if_t<std::is_floating_point_v<T>, T> as() const {
        return static_cast<T>(toFloat()) ;
    }

    template<typename T>
    std::enable_if_t<std::is_integral<T>::value && std::is_signed<T>::value, T> as() const {
        return static_cast<T>(toSignedInteger()) ;
    }

    template<typename T>
    std::enable_if_t<std::is_integral<T>::value && std::is_unsigned<T>::value, T> as() const {
        return static_cast<T>(toUnsignedInteger()) ;
    }

    template<typename T>
    std::enable_if_t<std::is_same<T, bool>::value, bool> as() const {
        return static_cast<T>(toBoolean()) ;
    }

    template<typename T>
    std::enable_if_t<std::is_same<T, std::string>::value, std::string> as() const {
        return static_cast<T>(toString()) ;
    }

    // Return the keys of an Object otherwise an empty list
    std::vector<std::string> keys() const;

    // length of object or array or string, zero otherwise
    size_t length() const;

    // Searches a member value given the key path. The path is of the form <member1>[.<member2>. ... <memberN>]
    // If this is not an object or the key is not found it returns false and <val> is untouched

    template<class T>
    bool lookup(const std::string &key_path, T &val) const {
        Variant v ;
        bool r = lookup(key_path, v) ;
        if ( r ) val = v.as<T>() ;
        return r ;
    }

    bool lookup(const std::string &key_path, Variant &val) const;

    // get value for key path and return default value if not found

    template<class T>
    T get(const std::string &key_path, const T &default_val) const {
        Variant v ;
        bool r = lookup(key_path, v) ;
        return (r) ? v.as<T>() : default_val ;
    }

    Variant value(const std::string &key, const Variant &defaultValue) const {
        Variant val ;
        if ( !lookup(key, val) ) return defaultValue ;
        else return val ;
    }

    const Variant &at(const std::string &key) const {
        return fetchConstKey(key) ;
    }

    // same as above but it returns a non-const reference or throws exception if not found or item is not an object
    Variant &at(const std::string &key)  {
        return fetchKey(key) ;
    }

    // return an element of an array
    const Variant &at(uint idx) const { return fetchIndex(idx) ; }

    // overloaded indexing operators
    const Variant &operator [] (const std::string &key) const {
        return fetchConstKey(key) ;
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
    void toJSON(std::ostream &strm) const;

    // export as JSON string

    std::string toJSON() const {
        std::ostringstream strm ;
        toJSON(strm) ;
        return strm.str() ;
    }

    // import from JSON stream

    static Variant fromJSON(std::istream &strm);

    static Variant fromJSON(const std::string &src) {
        std::istringstream strm(src) ;
        return fromJSON(strm) ;
    }

    // reads from a config file which is an extended version of json
    static Variant fromConfigFile(const std::string &path, const std::string &inc_path = {}) ;
    static Variant fromConfigString(const std::string &src, const std::string &inc_path = {}) ;

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

    static Variant parseJSONValue(JSONReader &reader);
    static Variant parseJSONObject(JSONReader &reader);
    static Variant parseJSONArray(JSONReader &reader);
    static std::string json_escape_string(const std::string &str);

    const Variant &fetchConstKey(const std::string &key) const;

    Variant &fetchKey(const std::string &key, bool safe = true );

    const Variant &fetchIndex(uint idx) const;

    Variant &fetchIndex(uint idx, bool safe = true);

    void destroy();

    void create(const Variant &other);

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

