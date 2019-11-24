#include <cvx/gfx/plot/markers.hpp>

using namespace std ;

namespace cvx { namespace gfx {

void SimpleShapeMarker::draw(cvx::gfx::Canvas &c, uint)
{
    if ( shape_ == None ) return ;

    Path p ;
    const double sz = sz_/2  ;
    const double szy = sz * sqrt((double)5)/2.0 ;

    switch ( shape_ ) {
        case Point:
            p.addEllipse(0, 0, 2, 2) ;
            break ;
        case Square:
            p.addRect(-sz, -sz, sz_, sz_) ;
            break ;
        case Circle:
            p.addEllipse(0, 0, sz, sz) ;
            break ;
        case Plus:
            p.moveTo(-sz, 0) ; p.lineToRel(sz_, 0) ;
            p.moveTo(0, -sz) ; p.lineToRel(0, sz_) ;
          break ;
        case XMark:
            p.moveTo(-sz, -sz) ; p.lineToRel(sz_, sz_) ;
            p.moveTo(-sz, sz) ; p.lineToRel(sz_, -sz_) ;
            break ;
        case Star:
            p.moveTo(-sz, 0).lineTo(sz, 0)
                    .moveTo(0, -sz).lineTo(0, sz)
                    .moveTo(-sz, -sz).lineTo(sz, sz)
                    .moveTo(sz, -sz).lineTo(-sz, sz);
          break ;
        case Diamond:
            p.moveTo(0, -sz).lineTo(-sz, 0).lineTo(0, sz).lineTo(sz, 0).closePath() ;
        break ;
        case TriangleUp:

            p.moveTo(0, -szy)
            .lineTo(-sz, szy)
            .lineTo(sz, szy)
            .lineTo(0, -szy) ;

          break ;
        case TriangleDown:
            p.moveTo(-sz, -szy).lineTo(0, szy).lineTo(sz, -szy).lineTo(-sz, -szy) ;
        break ;
            p.moveTo(-szy, 0)
            .lineTo(szy, -sz)
            .lineTo(szy, sz)
            .lineTo(-szy, 0) ;

          break ;
        case TriangleRight:
            p.moveTo(szy, 0)
            .lineTo(-szy, -sz)
            .lineTo(-szy, sz)
            .lineTo(szy, 0) ;
            break ;
       }

    c.save() ;
    c.setPen(*pen_) ;
    c.setBrush(*brush_) ;
    c.drawPath(p) ;
    c.restore() ;

}

void AdaptiveCircleMarker::draw(Canvas &c, uint idx)
{
    c.save() ;
    c.setPen(Pen(getOutlineColor(idx))) ;
    c.setBrush(SolidBrush(getFillColor(idx))) ;
    double radius = getRadius(idx) ;
    c.drawEllipse(0, 0, radius, radius) ;
    c.restore() ;

}

}}
