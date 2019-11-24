#ifndef CVX_GFX_PLOT_LINEGRAPH_HPP
#define CVX_GFX_PLOT_LINEGRAPH_HPP

#include <cvx/gfx/plot/graph.hpp>
#include <cvx/gfx/plot/markers.hpp>

namespace cvx { namespace gfx {

class LineGraph: public Graph {
public:
    LineGraph(const std::vector<double> &x, const std::vector<double> &y, const char *ps = nullptr) ;

    LineGraph &setErrors(const std::vector<double> &e) { e_ = e ; return *this; }

    Pen &pen() { return pen_ ; }
    LineGraph & setMarker(Marker *marker) { marker_.reset(marker) ; return *this ; }
    void setBrush(Brush *brush) {
        brush_.reset(brush) ;
    }

    BoundingBox getDataBounds() override;
    void draw(Canvas &c) override;
    void drawLegend(Canvas &c, double width, double height) override;

    void parseParamString(const char *ps) ;

private:

    Pen pen_ = Pen() ;
    std::unique_ptr<BrushBase> brush_ = std::unique_ptr<BrushBase>(new EmptyBrush) ;
    std::vector<double> x_, y_, e_ ;
    std::unique_ptr<Marker> marker_ ;

    Pen error_bar_pen_ = Pen() ;
    Pen error_bar_cap_pen_ = Pen();
    uint draw_error_bars_every_nth_ = 1 ;
    double error_bar_cap_width_ = 3.0 ;

} ;


}}



#endif
