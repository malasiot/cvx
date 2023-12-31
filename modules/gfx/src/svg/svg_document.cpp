#include <cvx/gfx/svg_document.hpp>
#include <cvx/gfx/canvas.hpp>

#include "svg_parser.hpp"
#include "svg_render_context.hpp"

#include <cvx/util/misc/strings.hpp>
#include <cvx/util/misc/base64.hpp>

#include <sstream>

using namespace std ;
using namespace svg ;
using namespace cvx::util ;

namespace cvx { namespace gfx {

SVGLoadException::SVGLoadException(const string &error_msg, size_t line, size_t col):
    msg_(error_msg), line_(line), col_(col) {}

std::string SVGLoadException::what() {
    ostringstream strm ;
    strm << "XML parse error: " << msg_ << " line: " << line_ << ", column: " << col_ ;
    return strm.str() ;
}

void SVGDocument::readStream(std::istream &strm) {
    SVGParser parser(*this) ;
    parser.parseStream(strm) ;
}

void SVGDocument::setLocalResourceFolder(const string &folder) {
    resource_folder_ = folder ;
}

void SVGDocument::registerNamedElement(const string &id, svg::Element *e) {
    elements_.insert({"#" + id, e}) ;
}


svg::Element *SVGDocument::resolve(const std::string &uri) const {
    if ( uri.empty() ) return nullptr ;
    auto it = elements_.find(uri) ;
    if ( it != elements_.end() ) {
        svg::Element *p = it->second ;
        return p ;
    }
    return nullptr ;
}

Image SVGDocument::loadImageResource(const string &uri, svg::Element *container) {
    auto it = cached_images_.find(container) ;
    if ( it != cached_images_.end() ) return it->second ;

    if ( startsWith(uri, "data:image/png;base64") ) {
        string png_data = base64_decode(uri.substr(22)) ;
        auto image =  Image::loadPNGBuffer(png_data) ;
        cached_images_[container] = image ;
        return image ;
    } else {
        if ( endsWith(uri, ".png") ) {
            Image im = Image::loadPNG(resource_folder_ + '/' + uri) ;
            return im ;
        }
        else if ( endsWith(uri, ".svg") ) {
            return Image() ;
        }

    }
}

void Canvas::drawSVG(const SVGDocument &doc)
{
    svg::RenderingContext ctx(*this) ;
    auto root = std::dynamic_pointer_cast<svg::SVGElement>(doc.getDOM()) ;
    ctx.render(*root) ;
}


}}
