#ifndef CVX_GFX_PLOT_AXIS_HPP
#define CVX_GFX_PLOT_AXIS_HPP

#include <string>
#include <memory>
#include <functional>
#include <cvx/gfx/canvas.hpp>

namespace cvx { namespace gfx {

using TickFormatter = std::function<std::string(double v, int idx)> ;


class Axis {
public:

    enum TicsPlacement { TicsInside, TicsOutside } ;

    Axis(double min_v, double max_v): min_v_(min_v), max_v_(max_v) {}

    void setTitle(const std::string &title) { title_ = title ; }
    void setTickFormatter(TickFormatter tf) { tick_formatter_ = tf ; }

protected:

    static TickFormatter nullFormatter, defaultFormatter ;

    // round to one decimal point
    double sround(double x);
    // compute normalized coordinates and associated tics
    void bounds(double sx, double xu, double xl, unsigned &nTics, double &rxu, double &rxl);
    // compute tic positions
    void computeAxisLayout(double ls, double wsize, double gscale);

    void paintLabel(Canvas &c, const std::string &label, double x, double y, bool rotate) ;

protected:
    double margin_ = 12.0 ;
    double label_sep_ = 40 ;
    bool is_log_ = false ;
    bool is_reversed_ = false ;
    double tic_size_ = 5 ;
    double label_offset_ = 5 ;
    double ticStep = 0.0 ;
    double title_offset_ = 5 ;
    double title_wrap_ = 100 ;
    std::string title_ ;
    TickFormatter tick_formatter_= defaultFormatter ;

    TicsPlacement tics_placement_ = TicsOutside ;

    Font label_font_ = Font("Arial", 12) ;
    Pen tics_pen_ = Pen() ;

    // layout data

    double step_, ls_ ;
    double vscale_, power_ ; // scaled applied to the displayed label values
    double min_v_, max_v_ ;
    double min_label_v_, max_label_v_ ; // minimum/maximum displayed label value
    double scale_, offset_  ;
    std::vector<std::string> labels_ ;
};

class XAxis: public Axis {
public:

    XAxis(double minV, double maxV): Axis(minV, maxV) {}

    void computeLayout(double wsize, double gscale);

    void draw(Canvas &canvas, double wsize, double gscale);

};

class YAxis: public Axis {
public:

    YAxis(double minV, double maxV): Axis(minV, maxV) {}

    void computeLayout(double wsize, double gscale);
    void draw(Canvas &canvas, double wsize, double gscale);

};

} }

#endif
