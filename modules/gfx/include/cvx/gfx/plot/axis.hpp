#ifndef CVX_GFX_PLOT_AXIS_HPP
#define CVX_GFX_PLOT_AXIS_HPP

#include <string>
#include <memory>

#include <cvx/gfx/canvas.hpp>
#include <cvx/gfx/plot/tics.hpp>

namespace cvx { namespace gfx {


class Axis {
public:

    enum TicsPlacement { TicsInside, TicsOutside } ;

    Axis() {
        grid_pen_.setDashArray({3, 10, 3, 10}) ;
    }

    Axis &setRange(double min_v, double max_v) {
        min_v_ = min_v ; max_v_ = max_v ;
        has_range_ = true ;
        return *this ;
    }

    Axis &setTitle(const std::string &title) { title_ = title ; return *this ; }
    Axis &setTickFormatter(TickFormatter tf) { tick_formatter_ = tf ; return *this ; }
    Axis &setLogarithmic(bool loga) { is_log_ = loga ; return *this ; }
    Axis &setGrid(bool grid) { draw_grid_ = grid ; return *this ; }
    Axis &setTickLocator(TickLocator *loc) { tick_locator_.reset(loc) ; return *this ;}

    double getScale() const { return scale_ ; }
    double getOffset() const { return offset_ ; }
    bool isLogarithmic() const { return is_log_ ; }

protected:

    static TickFormatter nullFormatter, defaultFormatter ;


    // compute tic positions
    void computeAxisLayout(double ls, double wsize, double gscale);

    Rectangle2d paintLabel(Canvas &c, const std::string &label, double x, double y, bool rotate) ;

protected:

    friend class Plot ;

    double margin_ = 12.0 ;
    double label_sep_ = 40 ;
    bool is_log_ = false ;
    bool is_reversed_ = false ;
    double tic_size_ = 5 ;
    double tic_minor_size_ = 3 ;
    double label_offset_ = 5 ;
    double title_offset_ = 5 ;
    double title_wrap_ = 100 ;
    bool draw_grid_ = false ;
    std::string title_ ;
    TickFormatter tick_formatter_= defaultFormatter ;
    std::unique_ptr<TickLocator> tick_locator_ = std::unique_ptr<TickLocator>(new AutoTickLocator()) ;

    TicsPlacement tics_placement_ = TicsOutside ;

    Font label_font_ = Font("Arial", 14) ;
    Font title_font_ = Font("Arial", 16) ;
    SolidBrush text_brush_ = SolidBrush(NamedColor::black()) ;
    Pen tics_pen_ = Pen(), grid_pen_ ;

    // layout data

    double step_, ls_ ;
    double vscale_, power_ ; // scaled applied to the displayed label values
    double min_v_, max_v_ ;
    double min_label_v_, max_label_v_ ; // minimum/maximum displayed label value
    double scale_, offset_  ;
    std::vector<std::string> labels_ ;
    bool has_range_ = false ;

};

class XAxis: public Axis {
public:

    void computeLayout(double wsize, double gscale);

    void draw(Canvas &canvas, double wsize, double hsize, double gscale);

    // from data coordinates to device coordinates
    double transform(double x) ;
};

class YAxis: public Axis {
public:

    void computeLayout(double wsize, double gscale);
    void draw(Canvas &canvas, double wsize, double hsize, double gscale);

    // from data coordinates to device coordinates
    double transform(double x) ;
};

} }

#endif
