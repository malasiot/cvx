#include <cvx/gfx/plot/axis.hpp>
#include <cvx/gfx/canvas.hpp>
#include <cvx/util/misc/strings.hpp>

using namespace std ;
namespace cvx { namespace gfx {

double Axis::sround(double x) {
    double s = 1.0 ;

    if ( fabs(x) < 1.0e-10 ) return 0.0 ;

    while ( x * s < 1.0 ) s *= 10 ;
    while ( x * s > 10.0 ) s /= 10 ;

    double sx = s * x ;

    if ( is_log_ ) {
        if ( sx >= 1.0 && sx < 2.0 ) return 1.0/s ;
        else if ( sx >= 2.0 && sx < 3.0 ) return 2.0/s ;
        else if ( sx >= 3.0 && sx <10.0 ) return 3.0/s ;
    }
    else {
        if ( sx >= 1.0 && sx < 2.0 ) return 1.0/s ;
        else if ( sx >= 2.0 && sx < 5.0 ) return 2.0/s ;
        else return 5.0/s ;
    }
}

void Axis::bounds(double sx, double xu, double xl, unsigned &nTics, double &rxu, double &rxl)
{
    int i, n = (int)(xu/sx) ;

    for(i=n+1 ; i>=n-1 ; i--) {
        if ( i * sx - xu <= 1.0e-4 ) break ;
    }

    rxu = i * sx ;

    n = (int)(xl/sx) ;

    for(i=n-1 ; i<=n+1 ; i++) {
        if ( i * sx - xl >= -1.0e-4 ) break ;
    }

    rxl = i * sx ;

    nTics = (rxl - rxu)/sx + 1.5;
}

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
        if ( _min <= 0 ) _min = log10(max_v_/10.0) ;
        else _min = log10(min_v_/10.0) ;

        _max = log10(_max) ;
    }

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

    // create labels

    labels_.resize(numTics) ;

    uint i ;

    if ( is_reversed_ ) {
        double v = max_label_v_ ;

        for(  i=0 ; i<numTics ; i++ ) {
            if ( fabs(v) < 1.0e-4 ) v = 0.0 ;
            labels_[i].assign(tick_formatter_(v, i)) ;
            v -= step_ ;
        }
    }
    else  {
        double v = min_label_v_ ;

        for(  i=0 ; i<numTics ; i++ ) {
            if ( fabs(v) < 1.0e-4 ) v = 0.0 ;
            labels_[i].assign(tick_formatter_(v, i)) ;
            v += step_ ;
        }
    }

    // compute transformation of this axis from data space to window space

    if ( is_reversed_ ) {
        scale_ = -(wsize - 2*margin_ *gscale)/(max_label_v_ - min_label_v_)*vscale_ ;
        offset_ = -scale_ * max_label_v_ / vscale_  + margin_ * gscale  ;
    }
    else
    {
        scale_ = double(wsize - 2*margin_ *gscale)/(max_label_v_ - min_label_v_)*vscale_ ;
        offset_ = -scale_ * min_label_v_ / vscale_  + margin_ * gscale  ;
    }

}

void Axis::paintLabel(Canvas &canvas,  const string &text, double x, double y, bool rotate)
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

    double lw = layout_mnt.width() ;
    double lh = layout_mnt.height() ;
    double soffset = 0.8*lh ;

    Text layout_expo;

    Font superf(label_font_) ;
     superf.setSize(label_font_.size()/2.0) ;

    if ( !expo.empty() ) {
        layout_expo.setText(expo);

        layout_expo.setFont(superf) ;


        double ew = layout_expo.width() ;
        double eh = layout_expo.height() ;

        lw += ew ;
        lh += soffset + eh - lh ;
    }


    Rectangle2d layout_rect(x - lw/2.0, y, lw, lh ) ;

    canvas.save() ;
    canvas.setFont(label_font_) ;
    canvas.setTextAlign(TextAlignBottom|TextAlignLeft) ;
    canvas.drawText(layout_mnt, layout_rect) ;

    if ( !expo.empty() ) {
        canvas.setFont(superf) ;
        canvas.setTextAlign(TextAlignTop|TextAlignRight) ;
        canvas.drawText(layout_expo, layout_rect) ;
    }

    canvas.restore() ;


}

void XAxis::computeLayout(double wsize, double gscale) {
    Text layout("-0.09") ;
    layout.setFont(label_font_) ;
    double maxLabelW = std::round(layout.width()) ;
    computeAxisLayout(maxLabelW, wsize, gscale) ;
}

void XAxis::draw(Canvas &canvas, double wsize, double gscale) {

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
    double labely = ticy + (( tics_placement_ == TicsInside ) ? - label_offset_ * gscale :  label_offset_ * gscale) ;

    for(  uint j=0 ; j<nTics ; j++ ) {
        double x1 = margin_ * gscale +  j * ts ;

        canvas.drawLine(x1, 0, x1, ticy) ;

        /*
          if ( j < nTicsX -1 && logX && fabs(stepx) == 1.0)
          {
            for( int k=2 ; k<10 ; k++ )
            {
              qreal offset = tsx*log10((double)k) ;

              painter.setPen(axisp) ;
              painter.drawLine(QPointF(x1 + offset, y1), QPointF(x1 + offset, y3)) ;

              if ( gridX )
              {
                painter.setPen(gridp) ;
                painter.drawLine(QPointF(x1 + offset, y1), QPointF(x1 + offset, y1 - sy)) ;
              }
            }
          }
          */

        if ( is_log_ )
        {
            /*
            QRectF powerRect ;
            powerRect = painter.boundingRect(powerRect, Qt::AlignLeft | Qt::AlignBottom, "10") ;

            QRectF boundRect ;
            painter.setFont(expoFont) ;
            boundRect = painter.boundingRect(boundRect, Qt::AlignLeft | Qt::AlignBottom, labelsX[j]) ;
            QRectF expoRect = QRectF(0, 0, boundRect.width(), boundRect.height()) ;

            powerRect.setWidth(powerRect.width() + boundRect.width()) ;
            powerRect.setHeight(powerRect.height() + boundRect.height()/2) ;

            QRectF layoutRect(x1 - powerRect.width()/2.0, y1 + 5 * scale,
            powerRect.width(), powerRect.height() ) ;

            expoRect.moveTopRight(layoutRect.topRight()) ;

            painter.setFont(labelFont) ;

            painter.drawText(layoutRect, Qt::AlignLeft | Qt::AlignBottom, "10") ;

            painter.setFont(expoFont) ;

            painter.drawText(expoRect, Qt::AlignLeft | Qt::AlignBottom, labelsX[j]) ;

            lbx = qMax(lbx, layoutRect.height()) ;

            painter.setFont(labelFont) ;
            */
        }
        else
        {
            paintLabel(canvas, labels_[j] + "10^2", x1,labely, false);
            /*Text label(labels_[j]) ;
            label.setFont(label_font_) ;

            Rectangle2d layout_rect(x1 - label.width()/2.0, labely, label.width(), label.height() ) ;

            canvas.drawText(label, layout_rect) ;


            lb = std::max(lb, label.height()) ;
            */
        }
    }



    // draw title

    if ( !title_.empty() ) {
        double x1 = margin_ * gscale +  s/2 ;
        double y1 = lb + labely + title_offset_ * gscale ;

        canvas.setTextAlign(TextAlignTop|TextAlignHCenter) ;
        Rectangle2d boundRect(x1 - title_wrap_/2, y1, title_wrap_, lb) ;

        canvas.drawText(title_, boundRect) ;
    }


    canvas.restore() ;
}

void YAxis::computeLayout(double wsize, double gscale) {
    Text layout("-0.09") ;
    layout.setFont(label_font_) ;
    double maxLabelH = std::round(layout.height()) ;
    computeAxisLayout(maxLabelH, wsize, gscale) ;
}

void YAxis::draw(Canvas &canvas, double wsize, double gscale) {

    unsigned s = wsize - 2 * margin_ * gscale ;
    unsigned nTics = labels_.size() ;

    double ts = s/(nTics - 1) ;

    canvas.save() ;
    canvas.setFont(label_font_) ;
    canvas.setPen(tics_pen_) ;
    canvas.setTextAlign(TextAlignLeft|TextAlignTop) ;

    canvas.drawLine(0, 0, 0, -wsize) ;

    double lb = 0 ;

    double ticx = ( tics_placement_ == TicsInside ) ?  tic_size_ * gscale :  -tic_size_ * gscale ;
    double labelx = ticx + (( tics_placement_ == TicsInside ) ?  label_offset_ * gscale :  -label_offset_ * gscale) ;

    for(  int j=0 ; j<nTics ; j++ ) {
        double y1 = -j * ts - margin_ * gscale /*- (margin_ * gscale + s) */;

        canvas.drawLine(0, y1, ticx, y1) ;

        /*
          if ( j < nTicsX -1 && logX && fabs(stepx) == 1.0)
          {
            for( int k=2 ; k<10 ; k++ )
            {
              qreal offset = tsx*log10((double)k) ;

              painter.setPen(axisp) ;
              painter.drawLine(QPointF(x1 + offset, y1), QPointF(x1 + offset, y3)) ;

              if ( gridX )
              {
                painter.setPen(gridp) ;
                painter.drawLine(QPointF(x1 + offset, y1), QPointF(x1 + offset, y1 - sy)) ;
              }
            }
          }
          */

        if ( is_log_ )
        {
            /*
            QRectF powerRect ;
            powerRect = painter.boundingRect(powerRect, Qt::AlignLeft | Qt::AlignBottom, "10") ;

            QRectF boundRect ;
            painter.setFont(expoFont) ;
            boundRect = painter.boundingRect(boundRect, Qt::AlignLeft | Qt::AlignBottom, labelsX[j]) ;
            QRectF expoRect = QRectF(0, 0, boundRect.width(), boundRect.height()) ;

            powerRect.setWidth(powerRect.width() + boundRect.width()) ;
            powerRect.setHeight(powerRect.height() + boundRect.height()/2) ;

            QRectF layoutRect(x1 - powerRect.width()/2.0, y1 + 5 * scale,
            powerRect.width(), powerRect.height() ) ;

            expoRect.moveTopRight(layoutRect.topRight()) ;

            painter.setFont(labelFont) ;

            painter.drawText(layoutRect, Qt::AlignLeft | Qt::AlignBottom, "10") ;

            painter.setFont(expoFont) ;

            painter.drawText(expoRect, Qt::AlignLeft | Qt::AlignBottom, labelsX[j]) ;

            lbx = qMax(lbx, layoutRect.height()) ;

            painter.setFont(labelFont) ;
            */
        }
        else
        {
            Text label(labels_[j]) ;
            label.setFont(label_font_) ;

            Rectangle2d layout_rect(labelx - label.width(), y1 - label.height()/2, label.width(), label.height() ) ;

            canvas.drawText(label, layout_rect) ;

            lb = std::max(lb, label.width()) ;
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
        canvas.drawText(title_, boundRect) ;
        canvas.restore() ;
    }


    canvas.restore() ;
}






}}
