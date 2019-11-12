#ifndef CVX_GFX_SVG_DOM_EXCEPTIONS_HPP
#define CVX_GFX_SVG_DOM_EXCEPTIONS_HPP

#include <string>
#include <stdexcept>

namespace svg {

class SVGDOMAttributeValueException {
public:
    SVGDOMAttributeValueException(const std::string &name, const std::string &val,
                                  const std::string &msg = std::string()): name_(name), val_(val), msg_(msg) {}
    std::string name_, val_, msg_ ;
} ;

class SVGDOMException: public std::runtime_error {
public:
    SVGDOMException(const std::string &msg): std::runtime_error(msg) {}
} ;

} //namespace svg




#endif

