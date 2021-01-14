#ifndef CVX_POLYGON_2D_HPP
#define CVX_POLYGON_2D_HPP

#include <cvx/geometry/rectangle.hpp>
#include <cvx/geometry/point_list.hpp>

namespace cvx {

template < class T, bool aligned >
class Polygon: public PointList<T, 2, aligned>
{
    using base_t = PointList<T, 2, aligned> ;

public:

    Polygon(const base_t &pts): base_t(pts) {}

    Polygon(uint n): base_t(n) {}

    Polygon(T *data, int n, bool row_major = true): base_t(makePointList<T, 2, aligned>(data, n, row_major)) {}

    T area() const {
        int n = this->size() ;

        T s = 0 ;
        for( uint i=0 ; i<n ; i++ )
        {
            uint i1 = (i+1)%n ;
            const Point<T, 2> &pi = base_t::at(i), &pi1 = this->at(i1) ;
            s += pi.x()*pi1.y() - pi1.x()*pi.y() ;
        }

        return 0.5*fabs(s) ;
    }

    Rectangle<T> boundingBox() const {
        Point<T, 2> pmin = this->at(0), pmax = this->at(0) ;

        for( uint i=1 ; i<this->size() ; i++ ) {
            pmin = min(pmin, this->at(i)) ;
            pmax = max(pmax, this->at(i)) ;
        }

        return Rectangle<T>(pmin, pmax) ;
    }

    bool contains(const Point<T, 2> &p) {
        int i, j, nvert = base_t::size() ;
        bool c = false ;
        for (i = 0, j = nvert-1; i < nvert; j = i++) {
            Point<T, 2> vi = base_t::mat_.row(i), vj = base_t::mat_.row(j) ;
            if ( ( (vi.y() > p.y() ) != ( vj.y() > p.y() ) ) &&
                 ( p.x() < ( vj.x() - vi.x() ) * ( p.y() - vi.y() ) / ( vj.y() - vi.y() ) + vi.x() ) )
                c = !c;
        }
        return c ;
    }

    bool contains(T x, T y) { return contains(Point<T, 2>(x, y)) ; }
} ;

typedef Polygon<double, true> Polygon2d ;
typedef Polygon<float, false> Polygon2f;

}

#endif
