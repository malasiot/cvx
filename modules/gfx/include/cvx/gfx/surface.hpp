#ifndef CVX_GFX_SURFACE_HPP
#define CVX_GFX_SURFACE_HPP

#include <cvx/gfx/impl/surface.hpp>

namespace cvx { namespace gfx {

class Surface: public detail::Surface {
protected:
    Surface(double w, double h, double dpix, double dpiy): dpix_(dpix), dpiy_(dpiy),
        width_(w), height_(h) {}
public:

    ~Surface() ;

    void flush() ;
    void clip(const Surface &other) ;

    double width() const { return width_ ; }
    double height() const { return height_ ; }
    double dpiX() const { return dpix_ ; }
    double dpiY() const { return dpiy_ ; }

private:
    double width_, height_, dpix_, dpiy_ ;

};

class ImageSurface: public Surface {
public:
    ImageSurface(int width, int height, double dpi_x = 96, double dpi_y = 96) ;

    Image getImage() const ;
};

class PDFSurface: public Surface {
public:
    PDFSurface(const std::string &path, int width, int height, double dpi_x = 300, double dpi_y = 300);
};

class RecordingSurface: public Surface {
public:
    RecordingSurface(double width, double height) ;
};

}}












#endif
