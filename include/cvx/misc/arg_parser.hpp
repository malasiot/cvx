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

typedef std::function<bool(std::istream &)> ValueParser ;

// Minimal command line argument parser
// Add options using option and positional functions, then call parse function
// The class does not act as a container of parsed values. Instead the values are stored in user variables which have to be valid when parse is called.
// Reading of arguments into values is performed internally be means of stream >> operations and therefore any variable type that supports this operator
// can be used.

class ArgumentParserException ;
class ArgumentParser ;

class ArgumentParser {
    class Value ;

public:

    ArgumentParser() {}

    // returns true if the command group has been succesfully matched
    bool matched() const { return is_matched_ ; }

    // current argument index
    size_t pos() const { return pos_ ; }

    bool has(const char *flag) ;

    class Option ;

    // add option that stores the arguments in the reference value
    // the template parameter can be any type defining the istream >> operator
    // flags is a list of acceptable flags (delimited by '|' or ',')
    // It is the user to set a default value to the variable before calling the parser. This will not be touched
    // if the option is not given on the command line otherwise one can use the .setDefault function

 #if 0
    template<class T>
    Option &option(const std::string &flags, T &value) {
        options_.emplace_back(flags, std::make_shared<ValueHolder<T>>(value)) ;
        return options_.back();
    }

    template<class T>
    Option &option(const std::string &flags, optional<T> &value) {
        options_.emplace_back(flags, std::make_shared<OptionalValueHolder<T>>(value)) ;
        return options_.back();
    }

    // add option that stores the arguments to a vector of values (i.e. to accept multiple args)
    // the template parameter can be any type defining the istream >> operator
    // flags is a list of acceptable flags (delimited by '|' or ',')

    template<class T>
    Option &option(const std::string &flags, std::vector<T> &value) {
        options_.emplace_back(flags, std::make_shared<ValueListHolder<T>>(value)) ;
        return options_.back() ;
    }
#endif
    // Use this for defining a custom argument parser
    // The parser is a lambda that should parse the value and store it to a captured variable
    // flags is a list of acceptable flags (delimited by '|' or ',')

 #if 0
    Option &option(const std::string &flags, ValueParser parser) {
        options_.emplace_back(flags, std::make_shared<ValueAdapter>(parser)) ;
        return options_.back() ;
    }
#endif

    Option &option(const std::string &flags) {
        options_.emplace_back(flags) ;
        return options_.back() ;
    }
    // Positional options

    template<class T>
    Option &positional(T &value) {
        positional_.emplace_back(std::make_shared<ValueHolder<T>>(value)) ;
        return positional_.back();
    }

    template<class T>
    Option &positional(std::vector<T> &value) {
        positional_.emplace_back(std::make_shared<ValueListHolder<T>>(value)) ;
        return positional_.back();
    }
#if 0
    Option &positional(ValueParser parser) {
        positional_.emplace_back(std::make_shared<ValueAdapter>(parser)) ;
        return positional_.back();
    }
#endif
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

        Option(const std::string& flags) ;

        template<typename T>
        Option &value(T &value, const char *name = "arg") {
            values_.emplace_back(std::make_shared<ValueHolder<T>>(value, name)) ;
            return *this ;
        }

        template<typename T>
        Option &value(optional<T> &value, const char *name = "arg") {
            values_.emplace_back(std::make_shared<OptionalValueHolder<T>>(value, name)) ;
            return *this ;
        }

        template<typename T>
        Option &value(std::vector<T> &value, bool not_empty = false, const char *name = "arg") {
            values_.emplace_back(std::make_shared<ValueListHolder<T>>(value, not_empty, name)) ;
            return *this ;
        }

        template<typename T>
        Option &implicit(T &value, const T &v) {
            values_.emplace_back(std::make_shared<ImplicitValue<T>>(value, v)) ;
            return *this ;
        }

        // Set option as required (not checked for positional arguments)
        Option &required(bool req = true) {
            is_required_ = req ;
            return *this ;
        }

        // Set description paragraph to be written on help output
        Option &description(const std::string &desc) {
            description_ = desc ;
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
        bool matches(const std::string &arg) const ;

        std::string formatOptionFlags() const ;
        void printHelp(std::ostream &strm) const;
        void printDescription(std::ostream &strm, uint first_col_width, uint line_length) const;

        std::vector<std::string> default_values_ ;
        std::vector<std::string> flags_;  // list of flags that this option accepts
        std::string description_ ;        // description of this arg
        std::string short_flag_ ;         // short flag selected from the flags
        std::string name_ ;               // the arg name that will appear after the flag in the help printout
        bool is_required_, is_positional_ ;
        std::vector<std::shared_ptr<Value>> values_ ;
        std::function<bool()> action_ = nullptr ;

        bool matched_ = false, is_group_switch_ = false ;
    };

private:

    class Value {
    public:
        Value(const char *name): name_(name) {}

        virtual bool read(size_t &c, const std::vector<std::string> &argv) = 0 ;

        virtual std::string name() const { return name_ ; }

    protected:
        std::string name_ ;
    };



    template <class T>
    class ValueHolder: public Value {
    public:
        ValueHolder(T &t, const char *name): val_(t), Value(name) {}

        std::string name() const { return name_ ; }

        bool read(size_t &c, const std::vector<std::string> &argv) {
            size_t argc = argv.size() ;
            if ( c >= argc ) return false ;

            std::stringstream strm ;

            std::string arg(argv[c]) ;
            if ( cvx::startsWith(arg, "-") ) return false ;

            strm << arg ;
            strm >> val_ ;
            if ( (bool)strm ) { c++ ; return true ; }
            return false ;
        }

    private:
        T &val_ ;
    };

    template <class T>
    class ImplicitValue: public Value {
    public:
        ImplicitValue(T &t, const T &imp): val_(t), implicit_(imp), Value("") {}

        std::string name() const { return {} ; }

        bool read(size_t &c, const std::vector<std::string> &argv) {
            val_ = implicit_ ;
            return true ;
        }

    private:
        T &val_ ;
        T implicit_ ;
    };

    template <class T>
    class OptionalValueHolder: public Value {
    public:
        OptionalValueHolder(optional<T> &t, const char *name): val_(t), Value(name) {}

        std::string name() const { return '[' + name_ + ']' ; }

        bool read(size_t &c, const std::vector<std::string> &argv) {
            size_t argc = argv.size() ;
            if ( c >= argc ) return true ;

            std::stringstream strm ;

            std::string arg(argv[c]) ;
            if ( cvx::startsWith(arg, "-") ) return true ;

            T val ;
            strm << arg ;
            strm >> val ;

            if ( (bool)strm ) {
                c++ ;
                val_ = val ;
                return true ;
            }
            return false ;
        }

    private:
        optional<T> &val_ ;
    };


    template <class T>
    class ValueListHolder: public Value {
    public:
        ValueListHolder(std::vector<T> &t, bool not_empty, const char *name): Value(name), list_(t),
            not_empty_(not_empty) {}

        bool read(size_t &c, const std::vector<std::string> &argv) {
            size_t argc = argv.size() ;
            if ( c >= argc  ) {
                if ( !not_empty_ ) return true ; // no more values empty list
                else return false ;
            }

            list_.clear() ;
            for( size_t pos = c ; pos < argc ; ++pos) {
                std::stringstream strm ;

                std::string arg(argv[pos]) ;
                if ( cvx::startsWith(arg, "-") ) {
                    if ( list_.empty() && not_empty_ ) return false ;
                    else return true ;
                }

                T val ;
                strm << arg ;
                strm >> val ;
                if ( (bool)strm ) { list_.emplace_back(val) ; ++c ; }
                else return false ;
            }
        }

         std::string name() const { if ( not_empty_ ) return '(' + name_ + ")+" ; else return '(' + name_ + ")*" ; }

    private:
        std::vector<T> &list_ ;
        bool not_empty_ = false ;
    };


    class ValueAdapter: public Value {
    public:
        ValueAdapter(ValueParser writer, const char *name = "arg"): writer_(writer), Value(name) {}

        bool read(std::istream &s) {
            return writer_(s) ;
        }

    private:
        ValueParser writer_ ;
    };


private:
    typedef std::vector<Option> Container ;

    void parse(const std::vector<std::string> &args) ;
    void consumeArg(const Container::iterator &match, const std::vector<std::string> &argv, size_t &c);
    Container::iterator findMatchingArg(const std::string &arg) ;
    void checkRequired() ;
    void setDefaults();
    uint getOptimalColumnWidth(uint line_length, uint min_description_length);
    void printOptions(std::ostream &strm, uint line_length = 80, uint min_description_width = 40) ;

    Container options_, positional_ ;
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
