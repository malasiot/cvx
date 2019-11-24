#ifndef CVX_GFX_PLOT_MARKERS_HPP
#define CVX_GFX_PLOT_MARKERS_HPP

#include <cvx/gfx/canvas.hpp>

namespace cvx { namespace gfx {

class Marker {
public:

    // the canvas has beed appropriately transformed to have the coordinate system at the center of marker
    // The idx is the index of the item in the data
    virtual void draw(Canvas &c, uint idx) = 0;
};

class SimpleShapeMarker: public Marker {
public:
    enum Shape { None, Point, Square, Circle, Diamond, Plus, XMark, Star, TriangleUp, TriangleDown, TriangleLeft, TriangleRight  } ;

    SimpleShapeMarker(Shape shape, double sz, const PenBase &pen, const BrushBase &brush):
        shape_(shape), sz_(sz), pen_(pen.clone()), brush_(brush.clone()) {}

    void draw(Canvas &c, uint idx) override ;

    PenBase *pen() { return pen_.get() ; }
    BrushBase *brush() { return brush_.get() ; }


protected:
    std::unique_ptr<PenBase> pen_ ;
    std::unique_ptr<BrushBase> brush_ ;
    double sz_ ;
    Shape shape_ ;
};


class AdaptiveCircleMarker: public Marker {
public:
    AdaptiveCircleMarker() {}

    void draw(Canvas &c, uint idx) override ;
    virtual double getRadius(uint idx) const = 0 ;
    virtual Color getFillColor(uint idx) const = 0 ;
    virtual Color getOutlineColor(uint idx) const = 0 ;
};


}}

#endif
