#ifndef CVX_GFX_PLOT_GRAPH_HPP
#define CVX_GFX_PLOT_GRAPH_HPP

#include <cvx/gfx/bbox.hpp>
#include <cvx/gfx/canvas.hpp>

namespace cvx { namespace gfx {
class Plot ;

class Graph {
public:

    virtual BoundingBox getDataBounds() = 0;
    virtual void draw(Canvas &c) = 0;
    virtual void drawLegend(Canvas &c, double width, double height) = 0;

protected:

    friend class Plot ;

    std::string title_ ;
    Plot *plot_ = nullptr ;
};

}}














#endif
