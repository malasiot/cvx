#ifndef CVX_GFX_BBOX_HPP
#define CVX_GFX_BBOX_HPP

#include <limits>
#include <algorithm>

namespace cvx { namespace gfx {

class BoundingBox {
public:

    BoundingBox() = default ;
    BoundingBox(double minx, double miny, double maxx, double maxy):
        minx_(minx), miny_(miny), maxx_(maxx), maxy_(maxy)  {
    }
    void extend(double x, double y) {
        minx_ = std::min(x, minx_) ;
        maxx_ = std::max(x, maxx_) ;
        miny_ = std::min(y, miny_) ;
        maxy_ = std::max(y, maxy_) ;
    }

    void extend(const BoundingBox &other) {
        extend(other.minX(), other.minY()) ;
        extend(other.maxX(), other.maxY()) ;
    }

    double minX() const { return minx_ ; }
    double minY() const { return miny_ ; }
    double maxX() const { return maxx_ ; }
    double maxY() const { return maxy_ ; }

private:
    double minx_ = std::numeric_limits<double>::max(),
           maxx_ = -std::numeric_limits<double>::max(),
           miny_ = std::numeric_limits<double>::max(),
           maxy_ = -std::numeric_limits<double>::max() ;
} ;


} }

#endif
