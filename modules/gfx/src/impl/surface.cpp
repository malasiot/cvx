#include <cvx/gfx/surface.hpp>

#include <cairo-pdf.h>

using namespace std ;

namespace cvx { namespace gfx {

Surface::~Surface()
{
    if ( surf_ != nullptr )
        cairo_surface_destroy(surf_) ;
}

void Surface::flush() {
    if ( surf_ != nullptr )
        cairo_surface_flush(surf_) ;
}

void Surface::clip(const Surface &other)
{
    cairo_surface_t *proxy_surf = cairo_surface_create_similar(surf_, CAIRO_CONTENT_COLOR_ALPHA, width_, height_) ;
    cairo_t *cr = cairo_create(proxy_surf) ;

    cairo_set_source_surface(cr, surf_, 0, 0);
    cairo_mask_surface(cr, other.surf_, 0, 0) ;
    cairo_fill(cr) ;
    cairo_surface_destroy(surf_) ;
    surf_ = proxy_surf ;
}

ImageSurface::ImageSurface(int w, int h, double dpix, double dpiy): Surface(w, h, dpix, dpiy)  {
    surf_ = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h) ;
}


RecordingSurface::RecordingSurface(double width, double height): Surface(width, height, 96, 96) {
    cairo_rectangle_t r{0, 0, width, height} ;
    surf_ = cairo_recording_surface_create(CAIRO_CONTENT_COLOR_ALPHA, &r) ;
}



Image ImageSurface::getImage() const
{
    char *src = (char *)cairo_image_surface_get_data(surf_) ;
    unsigned width = cairo_image_surface_get_width(surf_) ;
    unsigned height = cairo_image_surface_get_height(surf_) ;
    unsigned src_stride = cairo_image_surface_get_stride(surf_) ;
    cairo_format_t src_format = cairo_image_surface_get_format(surf_) ;

    Image im(width, height, ImageFormat::ARGB32) ;

    unsigned dst_stride = im.stride() ;
    char *dst, *p, *q ;
    dst = im.pixels() ;
    uint i, j ;

    if ( src_format == CAIRO_FORMAT_ARGB32 ) {
        for( i=0 ; i<height ; i++, dst += dst_stride, src += src_stride ) {
            for( j=0, p=src, q=dst ; j<width ; j++ ) {
#if __BYTE_ORDER == __LITTLE_ENDIAN
                char a = *p++, r = *p++, g = *p++, b = *p++ ;
#else

                char b = *p++, g = *p++, r = *p++, a = *p++ ;
#endif
                *q++ = a ; *q++ = r ; *q++ = g ; *q++ = b ;
            }
        }
    }

    return im ;
}

PDFSurface::PDFSurface(const string &fileName, int w, int h, double dpi_x, double dpi_y): Surface(w, h, dpi_x, dpi_y) {
    surf_ = cairo_pdf_surface_create(fileName.c_str(), w , h) ;
}

}}
