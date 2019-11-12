#ifndef CVX_GFX_SVG_DOCUMENT_HPP
#define CVX_GFX_SVG_DOCUMENT_HPP

#include <string>
#include <istream>
#include <memory>
#include <map>

#include <cvx/gfx/image.hpp>


namespace svg {
    class SVGElement ;
    class Element ;
    class SVGParser ;
}

namespace cvx { namespace gfx {

// Encapsulates the SVG dom
class SVGDocument {
public:

    void readStream(std::istream &strm) ;

    void setLocalResourceFolder(const std::string &folder) ;

    void registerNamedElement(const std::string &id, svg::Element *e);

    svg::Element *resolve(const std::string &uri) const ;
    Image loadImageResource(const std::string &uri, svg::Element *container) ;

    std::shared_ptr<svg::SVGElement> getDOM() const { return root_ ; }

private:

    friend class svg::SVGParser;

    std::shared_ptr<svg::SVGElement> root_ ;
    std::string resource_folder_ ;

protected:

    std::map<std::string, svg::Element *> elements_ ;
    std::map<svg::Element *, Image> cached_images_ ;
};


class SVGLoadException {
public:
    SVGLoadException(const std::string &error_msg, size_t line, size_t col) ;

    std::string what() ;
private:
    std::string msg_ ;
    size_t line_, col_ ;
};

}}


#endif
