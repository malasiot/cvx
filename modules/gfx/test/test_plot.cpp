#include <cvx/gfx/canvas.hpp>
#include <cvx/gfx/image.hpp>
#include <cvx/gfx/vector.hpp>

#include <cvx/gfx/bbox.hpp>

#include <cvx/util/misc/strings.hpp>

#include <cmath>
using namespace cvx::gfx ;
using namespace std ;


class Axis {
public:
    enum TicsPlacement { TicsInside, TicsOutside } ;

    void setTitle(const std::string &title) { title_ = title ; }

protected:
    // round to one decimal point
    double sround(double x) {
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

    void bounds(double sx, double xu, double xl, unsigned &nTics, double &rxu, double &rxl)
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


    void computeAxisLayout(double ls, double wsize, double gscale, double minV, double maxV) {

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

        double scale = 1.0 ;
        double power = 0 ;

        if ( !is_log_ ) {
            double v = max(fabs(minV), fabs(maxV)) ;
            while ( v * scale <= 0.1 )  { scale *= 10 ; ++power ; }
            while ( v * scale > 10.0 ) { scale /= 10 ; --power ; }
        }

        uint tics = numTics ;

        double _max = maxV, _min = minV ;

        if ( is_log_ )
        {
            if ( _min <= 0 ) _min = log10(maxV/10.0) ;
            else _min = log10(minV/10.0) ;

            _max = log10(_max) ;
        }

        double step, minLabelV, maxLabelV = maxV ;

        if ( ticStep == 0.0 ) {
             // recompute tic number and tic step to achieve normalized steps
            while (1)
            {

                step = sround((_max - _min)*scale/(tics-1)) ;

                bounds(step, _min*scale, _max*scale, numTics, minLabelV, maxLabelV) ;

                sep = (s - ((numTics-1)*ls))/(numTics-1) ;
                if ( sep > label_sep_ * gscale) break ;
                else tics -- ;

                if ( tics == 1 ) {
                    numTics = 2 ;
                    step = maxLabelV - minLabelV ;
                    break ;
                }
            }
        }
        else  {
            step = sround(ticStep*scale) ;
            bounds(step, _min*scale, _max*scale, numTics, minLabelV, maxLabelV) ;
        }

        // create labels

        labels_.resize(numTics) ;

        uint i ;

        if ( is_reversed_ ) {
            double v = maxLabelV ;

            for(  i=0 ; i<numTics ; i++ ) {
                if ( fabs(v) < 1.0e-4 ) v = 0.0 ;
                labels_[i].assign(cvx::util::format("%.2g", v)) ;
                v -= step ;
            }
        }
        else  {
            double v = minLabelV ;

            for(  i=0 ; i<numTics ; i++ ) {
                if ( fabs(v) < 1.0e-4 ) v = 0.0 ;
                labels_[i].assign(cvx::util::format("%.2g", v)) ;
                v += step ;
            }
        }

        // compute transformation of this axis from data space to window space

        if ( is_reversed_ ) {
            scale_ = -(wsize - 2*margin_ *scale)/(maxLabelV - minLabelV)*scale ;
            offset_ = -scale_ * maxLabelV / scale  + margin_ * scale  ;
        }
        else
        {
            scale_ = double(wsize - 2*margin_ *scale)/(maxLabelV - minLabelV)*scale ;
            offset_ = -scale_ * minLabelV / scale  + margin_ * scale  ;
        }

    }

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

    TicsPlacement tics_placement_ = TicsOutside ;

    Font label_font_ = Font("Arial", 12) ;
    Pen tics_pen_ = Pen() ;

    // layout data

    double minLabelV, maxLabelV ;
    double scale_, offset_  ;
    vector<string> labels_ ;
};


class XAxis: public Axis {
public:


    void computeLayout(double wsize, double gscale, double minV, double maxV) {
        Text layout("-0.09") ;
        layout.setFont(label_font_) ;
        double maxLabelW = std::round(layout.width()) ;
        computeAxisLayout(maxLabelW, wsize, gscale, minV, maxV) ;
    }

    void draw(Canvas &canvas, double wsize, double gscale) {

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
              Text label(labels_[j]) ;
              label.setFont(label_font_) ;

              Rectangle2d layout_rect(x1 - label.width()/2.0, labely, label.width(), label.height() ) ;

              canvas.drawText(label, layout_rect) ;


              lb = std::max(lb, label.height()) ;
          }
        }

        // draw label

        if ( !title_.empty() ) {
            double x1 = margin_ * gscale +  s/2 ;
            double y1 = lb + labely + title_offset_ * gscale ;

            canvas.setTextAlign(TextAlignTop|TextAlignHCenter) ;
            Rectangle2d boundRect(x1 - title_wrap_/2, y1, title_wrap_, lb) ;

            canvas.drawText(title_, boundRect) ;
          }


        canvas.restore() ;
    }


protected:

};

class YAxis: public Axis {
public:

    void computeLayout(double wsize, double gscale, double minV, double maxV) {
        Text layout("-0.09") ;
        layout.setFont(label_font_) ;
        double maxLabelH = std::round(layout.height()) ;
        computeAxisLayout(maxLabelH, wsize, gscale, minV, maxV) ;
    }

    void draw(Canvas &canvas, double wsize, double gscale) {

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


protected:

};

int main(int argc, char *argv[]) {

    ImageSurface is(1024, 512) ;
    Canvas canvas(is) ;

    canvas.setPen(Pen()) ;
    canvas.fill(NamedColor::white()) ;

    Font f("Arial", 32) ;
    f.setStyle(FontStyle::Italic) ;
    canvas.setFont(f) ;

    XAxis x_axis ;
    YAxis y_axis ;

    Matrix2d tr ;
    tr.translate(100, 400) ;

    canvas.setTransform(tr);
    x_axis.setTitle("Seasons");
    x_axis.computeLayout(500, 1.0, 0.0, 1.0) ;
    x_axis.draw(canvas, 500, 1) ;

    y_axis.setTitle("Values") ;
    y_axis.computeLayout(300, 1.0, -3.0e-4, 3.0e-4) ;
    y_axis.draw(canvas, 300, 1) ;


    is.flush() ;
    is.getImage().saveToPNG("/tmp/oo.png") ;
}
