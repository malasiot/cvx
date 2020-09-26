#include <cvx/gfx/qt_surface.hpp>

namespace cvx {
namespace gfx {

QtSurface::QtSurface(QPainter *p):
    ImageSurface(p->device()->width(), p->device()->height(), p->device()->logicalDpiX(), p->device()->logicalDpiY()),
    painter_(p)
{

}

void QtSurface::flush() {
    ImageSurface::flush() ;

    uchar *src = (uchar *)cairo_image_surface_get_data(surf_) ;
    unsigned width = cairo_image_surface_get_width(surf_) ;
    unsigned height = cairo_image_surface_get_height(surf_) ;
    unsigned src_stride = cairo_image_surface_get_stride(surf_) ;
    cairo_format_t src_format = cairo_image_surface_get_format(surf_) ;

    QImage image(width, height, QImage::Format_ARGB32) ;

    if ( src_format == CAIRO_FORMAT_ARGB32 ) {
        for( uint i=0 ; i<height ; i++,  src += src_stride ) {
            uchar *dst = image.scanLine(i), *p = src, *q = dst;
            for( uint j=0 ; j<width ; j++ ) {
#if __BYTE_ORDER == __LITTLE_ENDIAN
                char a = *p++, r = *p++, g = *p++, b = *p++ ;
#else

                char b = *p++, g = *p++, r = *p++, a = *p++ ;
#endif
                *q++ = a ; *q++ = r ; *q++ = g ; *q++ = b ;
            }
        }
    }

    painter_->drawImage(QPoint(0, 0), image) ;

}



}
}
