#pragma once

#include <cvx/misc/variant.hpp>

#include <istream>
#include <memory>

namespace cvx {

namespace detail {
class JSONTokenizer ;
}

class ConfigList ;

class Config {
public:
    Config() ;
    ~Config() ;

    void loadFile(const std::string &fpath, const std::string &inc_path = {}) ;
    void loadString(const std::string &str,  const std::string &inc_path = {}) ;

    bool hasKey(const std::string &path) ;

    bool value(const char *path, bool &value) const ;
    bool value(const char *path, int &value) const;
    bool value (const char *path, unsigned int &value) const;
    bool value (const char *path, long long &value) const;
    bool value (const char *path, unsigned long long &value) const;
    bool value (const char *path, float &value) const;
    bool value (const char *path, double &value) const;
    bool value (const char *path, std::string &value) const;

    ConfigList value(const char *path) ;

    template <class T>
    bool value(const std::string &key, T &v) const {
        return value(key.c_str(), v) ;
    }

    Config group(const char *path) const ;

    Config operator[](const char *path) const {
        return group(path) ;
    }

private:

    struct ParseContext {
        std::unique_ptr<detail::JSONTokenizer> tokenizer_ ;
        Variant var_ ;
        std::string inc_path_, path_ ;
    };

    Config(const Variant &v): container_(v) {}

    Variant container_ ;

    void parse(ParseContext &ctx, Variant &v) ;
    bool parseNameValue(ParseContext &ctx, Variant &v);
    Variant parseValue(ParseContext &ctx) ;
};

class ConfigParseException: public std::runtime_error {
public:
    ConfigParseException(const std::string &msg): std::runtime_error(msg) {}
};

class ConfigTypeException: public std::runtime_error {
public:
    ConfigTypeException(const std::string &msg): std::runtime_error("Wrong type for key: " + msg) {}
};

class ConfigKeyException: public std::runtime_error {
public:
    ConfigKeyException(const std::string &msg): std::runtime_error("Invalid key: " + msg) {}
};
}
