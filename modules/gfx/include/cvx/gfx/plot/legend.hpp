#ifndef CVX_GFX_PLOT_LEGEND_HPP
#define CVX_GFX_PLOT_LEGEND_HPP

#include <cvx/gfx/canvas.hpp>


namespace cvx { namespace gfx {
class Plot ;
class Graph ;

class Legend {
public:
    enum Placement { None, TopRight, TopLeft, TopCenter, BottomLeft, BottomCenter, BottomRight, CenterLeft, CenterRight } ;

    Legend &setPlacement(Placement p) { placement_ = p ; return *this ;}
protected:

    friend class Plot ;

    Legend() {}

    void draw(const std::vector<std::unique_ptr<Graph>> &graphs, Canvas &c, double w, double h) ;

private:

    double max_label_width_ = 150 ;
    double preview_width_ = 30 ;
    double min_row_height_ = 30;
    double padding_ = 6 ;
    double margin_ = 4 ;
    Placement placement_ = TopRight ;

    Font label_font_ = Font("Arial", 10) ;
    SolidBrush label_brush_ = SolidBrush(NamedColor::black()) ;
    SolidBrush bg_brush_ = SolidBrush(NamedColor::white()) ;
    Pen bg_pen_ = Pen() ;
} ;


}}

#endif
