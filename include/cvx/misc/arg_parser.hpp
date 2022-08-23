#ifndef CVX_ARG_PARSER_HPP
#define CVX_ARG_PARSER_HPP

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <istream>
#include <iterator>
#include <sstream>
#include <limits>

#include <cvx/misc/optional.hpp>
#include <cvx/misc/strings.hpp>


namespace cvx {

// by default option values are parsed using >>
// For custom types you should specialise the template below

template <typename T>
struct ValueParser {
    bool parse(const std::string &txt, T &val) {
        std::stringstream strm ;
        strm << txt ;
        strm >> val ;
        return  (bool)strm ;
    }
};

// Minimal command line argument parser
// Add options using option and positional functions, then call parse function
// The class does not act as a container of parsed values. Instead the values are stored in user variables which have to be valid when parse is called.
// Reading of arguments into values is performed internally be means of stream >> operations and therefore any variable type that supports this operator
// can be used. Otherwise the option argument can be parsed by passing a lambda of type ValueParser

class ArgumentParserException ;
class ArgumentParser ;

class Value {
public:
    Value(bool parse_arg = true): parse_arg_(parse_arg) {} ;
    virtual bool read(const std::string &arg) = 0 ;
    bool parseArg() const { return parse_arg_ ; }
private:
    bool parse_arg_ = true ;
};

template<class T>
bool default_parse_arg(T &val, const std::string &arg) {
    std::stringstream strm ;
    strm << arg ;
    strm >> val ;
    return  (bool)strm ;
}


template <class T>
class ValueHolder: public Value {
public:
    ValueHolder(T &t): val_(t), Value(true) {}

    bool read(const std::string &arg) {
        ValueParser<T> parser ;
        return parser.parse(arg, val_) ;
    }

private:
    T &val_ ;
};


template <>
class ValueHolder<bool>: public Value {
public:
    ValueHolder(bool &t): Value(false), val_(t) {}

    bool read(const std::string &) {
        val_ = true ;
        return true ;
    }

private:
    bool &val_ ;
};

template <class T>
class ValueListHolder: public Value {
public:
    ValueListHolder(std::vector<T> &t, const char *delimeters = ","): list_(t), delimeters_(delimeters) {}

    bool read(const std::string &arg) {
        ValueParser<T> parser ;

        size_t count = 0 ;
        auto tokens = split(arg, delimeters_) ;
        for( const auto &token: tokens ) {
            T val ;
            if ( parser.parse(token, val) ) {
                list_.emplace_back(val) ;
                ++count ;
            }
        }

        return count ;
    }

private:
    std::vector<T> &list_ ;
    const char *delimeters_ ;
};

bool is_option(const std::string &v) {
    if ( v.size() < 2 ) return false ;
    if ( v[0] != '-' ) return false ;
    if ( v[1] == '-' || std::isalpha(v[1]) ) return true ;
    return false ;
}

class PositionalValue {
public:
    PositionalValue() = default ;
    virtual bool read(const char *argv[], size_t argc, size_t &c) = 0 ;
};

template <class T>
class PositionalValueHolder: public PositionalValue {
public:
    PositionalValueHolder(T &t): val_(t) {}

    bool read(const char *argv[], size_t argc, size_t &c) {
        if ( c == argc || is_option(argv[c]) ) return false ;

        std::string arg = argv[c] ;

        ValueParser<T> parser ;
        if ( parser.parse(arg, val_) ) {
            ++c ;
            return true ;
        }

        return false ;
    }

private:
    T &val_ ;
};

template <class T>
class PositionalValueListHolder: public PositionalValue {
public:
    PositionalValueListHolder(std::vector<T> &t): list_(t) {}

    bool read(const char *argv[], size_t argc, size_t &c) {
        ValueParser<T> parser ;
        while ( c < argc && !is_option(argv[c]) ) {
            T val ;
            std::string arg = argv[c] ;
            if ( parser.parse(arg, val) ) {
                ++c ;
                list_.emplace_back(val) ;
            }
        }

        return true ;
    }

private:
    std::vector<T> &list_ ;
};


class ArgumentParser {

public:

    ArgumentParser() {}

    // returns true if the command group has been succesfully matched
    bool matched() const { return is_matched_ ; }

    // current argument index
    size_t pos() const { return pos_ ; }

    bool has(const char *flag) ;

    class Option ;
    class Positional ;

    // add option with optional list of arguments
    // flags is a list of acceptable flags (delimited by '|' or ',')
    // Any text after the list of flags (seprated by space or tab) is a description of the arguments that is printed in the help message.
    // e.g. "-f|--flag [<x>]" will use "-f" and "--flag" as option names.
    //
    // Add argument of type T. If the option is parsed correctly the passed variable will contain the value parsed.
    // Any type readable from std::istream may be used. For more complex values the << operator may be ovveriden or
    // the method taking as input a ValueParser can be used. If the option is not present the variable will be untouched.
    // Its the users responsibility to set default values in this case before calling this method.
    // For options taking no value you should supply a boolean variable that will be set to tree when the option is present.

    template<class T>
    Option &option(const std::string &flags, T &value, const std::string description = std::string()) {
        options_.emplace_back(flags, value, description) ;
        return options_.back() ;
    }

    // for options expecting a list items should be delimeted by ','. Alternatively use delimeter function to supply custom delimeters
    template<class T>
    Option &option(const std::string &flags, const std::vector<T> &value, const std::string description = std::string()) {
        options_.emplace_back(flags, value, description) ;
        return options_.back() ;
    }
    // Positional arguments

    template<class T>
    Positional &positional(const std::string &name, T &value, const std::string &desc = std::string()) {
        positional_.emplace_back(name, value, desc) ;
        return positional_.back();
    }


    template<class T>
    Positional &positional(const std::string &name, std::vector<T> &value, const std::string &desc = std::string()) {
        positional_.emplace_back(name, value, desc) ;
        return positional_.back();
    }

    // Parse the command line. In case of failure an exception is thrown. Last argument is the first command line argument to consider
    void parse(size_t argc, const char *argv[], size_t c = 1) ;

    void printUsage(std::ostream &strm, uint line_length = 80, uint min_description_width = 40) ;

    // set the description of the command printed first in the usage print out.
    void description(const std::string &desc) {
        description_ = desc ;
    }

    // set the caption printed before the list of options:
    void optionsCaption(const std::string &caption) {
        options_caption_ = caption ;
    }

    // set the text printed after the list of options:
    void epilog(const std::string &epilog) {
        epilog_ = epilog ;
    }

    class Option {
    public:

        template<typename T>
        Option(const std::string& flags, T &value, const std::string &desc): description_(desc) {
            parseFlags(flags) ;
            value_ = std::make_shared<ValueHolder<T>>(value) ;
        }

        template<typename T>
        Option(const std::string& flags, std::vector<T> &value, const std::string &desc): description_(desc) {
            parseFlags(flags) ;
            value_ = std::make_shared<ValueListHolder<T>>(value, ldel_) ;
        }


       // If the option is present set variable "value" equal to "v".

        Option &implicit(const std::string &v) {
            implicit_value_ = v ;
            return *this ;
        }

        // Set option as required (not checked for positional arguments)
        Option &required(bool req = true) {
            is_required_ = req ;
            return *this ;
        }

        Option &setListDelimeters(const char *d) {
            ldel_ = d ;
            return *this ;
        }

        // set a functor to be called after the option has been succefully parsed. Return true to continue with the rest of the args, false to stop.
        // This is usefull to parse subcommands. Declare a positional argument and then on setAction callback call the appropriate ArgumentParser for the subcommand
        // based on the positional variable value

        Option &action(std::function<bool()> cb) {
            action_ = cb ;
            return *this ;
        }

    protected:

        friend class ArgumentParser ;
        bool matches(const std::string &arg, std::string &val) const ;
        void parseFlags(const std::string &f) ;

        std::string formatOptionFlags() const ;
        void printHelp(std::ostream &strm) const;
        void printDescription(std::ostream &strm, uint first_col_width, uint line_length) const;

        std::vector<std::string> flags_;  // list of flags that this option accepts
        std::string description_ ;        // description of this arg
        std::string short_flag_ ;         // short flag selected from the flags
        std::string implicit_value_ ;
        std::string name_ ;
        bool is_required_ = false ;
        std::shared_ptr<Value> value_ ;
        std::function<bool()> action_ = nullptr ;
        const char *ldel_ = "," ;

        bool matched_ = false ;
    };

    class Positional {
    public:

        template<typename T>
        Positional(const std::string &name, T &value, const std::string &desc): name_(name), desc_(desc) {
            value_ = std::make_shared<PositionalValueHolder<T>>(value) ;
        }

        template<typename T>
        Positional(const std::string &name, std::vector<T> &value, const std::string &desc): name_(name), desc_(desc) {
            value_ = std::make_shared<PositionalValueListHolder<T>>(value) ;
        }

        const std::string &name() const { return name_ ; }
        const std::string &desc() const { return desc_ ; }

        Positional &action(std::function<bool()> cb) {
            action_ = cb ;
            return *this ;
        }

        Positional &required(bool req = true) {
            required_ = req ;
            return *this ;
        }

    protected:

        friend class ArgumentParser ;

        std::shared_ptr<PositionalValue> value_ ;
        std::string name_, desc_ ;
        std::function<bool()> action_ ;
        bool required_ = false ;

        bool matched_ = false ;
    };

private:

private:
    using OptionsContainer = std::vector<Option> ;
    using PositionalContainer = std::vector<Positional> ;

    void parse(const std::vector<std::string> &args) ;
    void consumeArg(Option &match, const std::string &val, const char *argv[], size_t argc, size_t &c);
    void consumePositionalArg(Positional &match, const char *argv[], size_t argc, size_t &c);
    OptionsContainer::iterator findMatchingArg(const std::string &arg, std::string &val) ;
    void checkRequired() ;
    void setDefaults();
    uint getOptimalColumnWidth(uint line_length, uint min_description_length);
    void printOptions(std::ostream &strm, uint line_length = 80, uint min_description_width = 40) ;

    OptionsContainer options_ ;
    PositionalContainer positional_ ;
    std::string options_caption_, description_, epilog_ ;
    std::string prog_ ;
    std::string usage_ ;

    bool is_matched_ = false ;
    size_t pos_ = 0 ;
};


class ArgumentParserException: public std::exception {
public:
public:
    ArgumentParserException(ArgumentParser &group, const std::string& msg): msg_(msg), group_(group) {}

    virtual const char*  what() const noexcept {
        return msg_.c_str();
    }

    ArgumentParser &group() { return group_ ; }

private:
    std::string msg_ ;
    ArgumentParser &group_ ;
};

class InvalidOption: public ArgumentParserException {
public:
    InvalidOption(ArgumentParser &group, const std::string& option)
        : ArgumentParserException(group, "Option \'" + option + "\' does not exist")
    {
    }
};

class IncorrectArguments: public ArgumentParserException {
public:
    IncorrectArguments(ArgumentParser &group, const std::string& option)
        : ArgumentParserException(group, "Option \'" + option + "\' has inccorect arguments")
    {
    }
};

class MissingRequiredOption: public ArgumentParserException {
public:
    MissingRequiredOption(ArgumentParser &group, const std::string& option)
        : ArgumentParserException(group, "Required option \'" + option + "\' is missing")
    {
    }
};

class InvalidCommandGroup: public ArgumentParserException {
public:
    InvalidCommandGroup(ArgumentParser &parent_group, const std::string& group)
        : ArgumentParserException(parent_group, "Invalid command group \'" + group + "\'")
    {
    }
};

}
#endif
