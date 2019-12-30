#include <cvx/gfx/plot/axis.hpp>
#include <cvx/gfx/canvas.hpp>
#include <cvx/util/misc/format.hpp>

using namespace std ;
namespace cvx { namespace gfx {

TickFormatter Axis::nullFormatter =
           [](double v, int idx) { return std::string() ; } ;

TickFormatter Axis::defaultFormatter =
           [](double v, int idx) {
              return cvx::util::format("%.2g", v) ;
    };


void Axis::computeAxisLayout(double ls, double wsize, double gscale) {

    ls_ = ls ;
    unsigned numTics = 2 ;

    double s = wsize - 2*margin_ * gscale - ls ;
    double sep ;

    // Compute number of tics for each axis based on the window dimensions

    while (1) {
        sep = (s - ((numTics-1)*ls))/(numTics-1) ;
        if ( sep < label_sep_ * gscale ) break ;
        else numTics ++ ;
    }

    // compute scaling factor of displayed labels

    vscale_ = 1.0 ;
#if 0
    power_ = 0 ;

    if ( !is_log_ ) {
        double v = max(fabs(min_v_), fabs(max_v_)) ;
        while ( v * vscale_ <= 0.1 )  { vscale_ *= 10 ; ++power_ ; }
        while ( v * vscale_ > 10.0 ) { vscale_ /= 10 ; --power_ ; }
    }
#endif

    uint tics = numTics ;

    double _max = max_v_, _min = min_v_ ;

    if ( is_log_ )
    {
        if ( _min <= 0 ) throw std::runtime_error("log scale for negative number") ;
        else _min = log10(min_v_) ;

        _max = log10(_max) ;
    }

    vector<double> tic_locations_ ;
    tick_locator_->compute(_min, _max, numTics, is_log_ ? 1.0 : 0.0, min_label_v_, max_label_v_, tic_locations_) ;

    /*
    max_label_v_ = max_v_ ;


    if ( ticStep == 0.0 ) {
        // recompute tic number and tic step to achieve normalized steps
        while (1)
        {

            step_ = sround((_max - _min)*vscale_/(tics-1)) ;

            bounds(step_, _min*vscale_, _max*vscale_, numTics, min_label_v_, max_label_v_) ;

            sep = (s - ((numTics-1)*ls))/(numTics-1) ;
            if ( sep > label_sep_ * gscale) break ;
            else tics -- ;

            if ( tics == 1 ) {
                numTics = 2 ;
                step_ = max_label_v_ - min_label_v_ ;
                break ;
            }
        }
    }
    else  {
        step_ = sround(ticStep*vscale_) ;
        bounds(step_, _min*vscale_, _max*vscale_, numTics, min_label_v_, max_label_v_) ;
    }
*/

    numTics = tic_locations_.size() ;

    // create labels

    labels_.resize(numTics) ;

    uint i ;

    for( i=0 ; i<numTics ; i++ ) {
        labels_[i].assign(tick_formatter_(tic_locations_[i], i)) ;
    }


    // compute transformation of this axis from data space to window space

    if ( is_reversed_ ) {
        scale_ = (wsize - 2*margin_ *gscale)/(max_label_v_ - min_label_v_)*vscale_ ;
        offset_ = -scale_ * max_label_v_ / vscale_  + margin_ * gscale  ;
    }
    else
    {
        scale_ = double(wsize - 2*margin_ *gscale)/(max_label_v_ - min_label_v_)*vscale_ ;
        offset_ = -scale_ * min_label_v_ / vscale_  + margin_ * gscale  ;
    }

}

Rectangle2d Axis::paintLabel(Canvas &canvas,  const string &text, double x, double y, bool rotate)
{
    string mnt, expo ;
    size_t spos = text.find('^') ;
    if ( spos != string::npos )  {
        mnt = text.substr(0, spos) ;
        expo = text.substr(spos+1) ;
    } else {
        mnt = text ;
    }

    Text layout_mnt(mnt) ;
    layout_mnt.setFont(label_font_) ;

    float xoffset = label_font_.size()/8 ; // better to get this from glyph advance

    double lw = layout_mnt.width() ;
    double lh = layout_mnt.height() ;
    double soffset = 0.66*lh ;

    Text layout_expo;

    Font superf(label_font_) ;
     superf.setSize(0.58 * label_font_.size()) ;

    if ( !expo.empty() ) {
        layout_expo.setText(expo);
        layout_expo.setFont(superf) ;

        double ew = layout_expo.width() ;
        double eh = layout_expo.height() ;

        lw += ew + xoffset;
        lh += soffset + eh - lh ;
    }

    double xf, yf ;
    if ( !rotate ) {
        xf = x - lw/2.0 ;
        yf = y ;
    } else {
       xf = x - lw ;
       yf = y - lh/2 ;
    }

    Rectangle2d layout_rect(xf, yf, lw, lh) ;

    canvas.save() ;
    canvas.setBrush(text_brush_) ;
    canvas.setPen(EmptyPen()) ;
    canvas.setFont(label_font_) ;
    canvas.setTextAlign(TextAlignBottom|TextAlignLeft) ;
    canvas.drawText(layout_mnt, layout_rect) ;

    if ( !expo.empty() ) {
        canvas.setFont(superf) ;
        canvas.setTextAlign(TextAlignTop|TextAlignRight) ;
        canvas.drawText(layout_expo, layout_rect) ;
    }

    canvas.restore() ;

    return layout_rect ;
}

void XAxis::computeLayout(double wsize, double gscale) {
    Text layout("-0.09") ;
    layout.setFont(label_font_) ;
    double maxLabelW = std::round(layout.width()) ;
    computeAxisLayout(maxLabelW, wsize, gscale) ;
}

void XAxis::draw(Canvas &canvas, double wsize, double hsize, double gscale) {

    unsigned s = wsize - 2 * margin_ * gscale ;
    unsigned nTics = labels_.size() ;

    double ts = s/(nTics - 1) ;

    canvas.save() ;
    canvas.setFont(label_font_) ;
    canvas.setPen(tics_pen_) ;
    canvas.setTextAlign(TextAlignLeft|TextAlignTop) ;

    canvas.drawLine(0, 0, wsize, 0) ;

    double lb = 0 ;

    double ticy = ( tics_placement_ == TicsInside ) ? - tic_size_ * gscale :  tic_size_ * gscale ;
    double ticy_minor = ( tics_placement_ == TicsInside ) ? - tic_minor_size_ * gscale :  tic_minor_size_ * gscale ;
    double labely = ticy + (( tics_placement_ == TicsInside ) ? - label_offset_ * gscale :  label_offset_ * gscale) ;

    // draw tics

    for(  uint j=0 ; j<nTics ; j++ ) {
        double x1 = margin_ * gscale +  j * ts ;

        canvas.drawLine(x1, 0, x1, ticy) ;

        if ( is_log_ && j < nTics - 1) { // minor
            for( int k=2 ; k<10 ; k++ ) {
              double offset = ts*log10((double)k) ;
              canvas.drawLine(x1 + offset, 0, x1 + offset, ticy_minor) ;
            }
        }

        if ( is_log_ )
        {
            Rectangle2d r = paintLabel(canvas, "10^" + labels_[j], x1,labely, false);
            lb = std::max(lb, r.height()) ;
        }
        else
        {
            Rectangle2d r = paintLabel(canvas, labels_[j], x1,labely, false);
            lb = std::max(lb, r.height()) ;
        }

        if ( draw_grid_ ) {
            canvas.save() ;
            canvas.setPen(grid_pen_) ;
            canvas.drawLine(x1, 0, x1, -hsize) ;
            canvas.restore() ;
        }
    }


    // draw title

    if ( !title_.empty() ) {
        double x1 = margin_ * gscale +  s/2 ;
        double y1 = lb + labely + title_offset_ * gscale ;

        canvas.setTextAlign(TextAlignTop|TextAlignHCenter) ;
        Rectangle2d boundRect(x1 - title_wrap_/2, y1, title_wrap_, lb) ;

        canvas.save() ;
        canvas.setFont(title_font_) ;
        canvas.setBrush(text_brush_) ;
        canvas.setPen(EmptyPen()) ;
        canvas.drawText(title_, boundRect) ;
        canvas.restore() ;
    }


    canvas.restore() ;
}

void YAxis::computeLayout(double wsize, double gscale) {
    Text layout("-0.09") ;
    layout.setFont(label_font_) ;
    double maxLabelH = std::round(layout.height()) ;
    computeAxisLayout(maxLabelH, wsize, gscale) ;
}

void YAxis::draw(Canvas &canvas, double wsize, double hsize, double gscale) {

    unsigned s = hsize - 2 * margin_ * gscale ;
    unsigned nTics = labels_.size() ;

    double ts = s/(nTics - 1) ;

    canvas.save() ;
    canvas.setFont(label_font_) ;
    canvas.setPen(tics_pen_) ;
    canvas.setTextAlign(TextAlignLeft|TextAlignTop) ;

    canvas.drawLine(0, 0, 0, -hsize) ;

    double lb = 0 ;

    double ticx = ( tics_placement_ == TicsInside ) ?  tic_size_ * gscale :  -tic_size_ * gscale ;
    double ticx_minor = ( tics_placement_ == TicsInside ) ?  tic_minor_size_ * gscale :  -tic_minor_size_ * gscale ;
    double labelx = ticx + (( tics_placement_ == TicsInside ) ?  label_offset_ * gscale :  -label_offset_ * gscale) ;

    for(  int j=0 ; j<nTics ; j++ ) {
        double y1 = -j * ts - margin_ * gscale /*- (margin_ * gscale + s) */;

        canvas.drawLine(0, y1, ticx, y1) ;

        if ( is_log_ && j < nTics - 1) { // minor
            for( int k=2 ; k<10 ; k++ ) {
              double offset = ts*log10((double)k) ;
              canvas.drawLine(0, y1 + offset, ticx_minor, y1 + offset) ;
            }
        }

        if ( is_log_ )
        {
            Rectangle2d r = paintLabel(canvas, "10^" + labels_[j], labelx, y1, true);
            lb = std::max(lb, r.width()) ;
        }
        else
        {
            Rectangle2d r = paintLabel(canvas, labels_[j], labelx, y1, true);
            lb = std::max(lb, r.width()) ;
        }

        if ( draw_grid_ ) {
            canvas.save() ;
            canvas.setPen(grid_pen_) ;
            canvas.drawLine(0, y1, wsize, y1) ;
            canvas.restore() ;
        }

    }

    if ( !title_.empty() ) {
        double y1 = -margin_ * gscale -  s/2 ;
        double x1 =  labelx - lb - title_offset_ * gscale ;

        Matrix2d tr ;
        tr.rotate(-M_PI/2.0, Vector2d(x1, y1)) ;

        canvas.save() ;
        canvas.setTransform(tr);

        canvas.setTextAlign(TextAlignHCenter|TextAlignBottom) ;

        const double bs = 2 * lb ; // 2 lines ? (font height)
        Rectangle2d boundRect(x1 - title_wrap_/2, y1 - bs, title_wrap_, bs) ;
        //    canvas.drawRect(boundRect) ;

        canvas.setFont(title_font_) ;
        canvas.setBrush(text_brush_) ;
        canvas.setPen(EmptyPen()) ;
        canvas.drawText(title_, boundRect) ;
        canvas.restore() ;

    }


    canvas.restore() ;
}

double XAxis::transform(double x) {
    if ( is_log_ ) {
        if ( x > 0.0 ) x = log10(x) ;
        else throw runtime_error("log scale needs positive numbers") ;
    }
    return scale_ * x + offset_ ;
}

double YAxis::transform(double y) {
    if ( is_log_ ) {
        if ( y > 0.0 ) y = log10(y) ;
        else throw runtime_error("log scale needs positive numbers") ;
    }
    return -(scale_ * y + offset_) ;
}



}}
