#include <cvx/gfx/plot/plot.hpp>

using namespace std ;

namespace cvx { namespace gfx {

void Plot::draw(Canvas &c, double w, double h)
{
    x_axis_.setRange(data_bounds_.minX(), data_bounds_.maxX()) ;
    y_axis_.setRange(data_bounds_.minY(), data_bounds_.maxY()) ;

    x_axis_.computeLayout(w, 1.0) ;
    y_axis_.computeLayout(h, 1.0) ;

    x_axis_.draw(c, w, h, 1.0);
    y_axis_.draw(c, w, h, 1.0);

    c.drawLine(0, -h, w, -h) ;
    c.drawLine(w, 0, w, -h) ;

    for( auto &g: graphs_ ) {
        g->draw(c) ;
    }

    if ( show_legend_ )
        drawLegend(c, w, h) ;

}

void Plot::drawLegend(Canvas &c, double w, double h) {
    size_t nGraphs = graphs_.size() ;

    double lw = 0, lh = 0, ox, oy ;

    vector<Text> labels ;

    for( size_t i=0 ; i<nGraphs ; i++ ) {
        Text t ;
        t.setText(graphs_[i]->title_) ;
        t.setWrapWidth(legend_max_label_width_) ;
        t.setFont(legend_font_) ;
        t.updateLayout() ;

        lw = std::max(lw, t.width()) ;
        lh += std::max(legend_min_row_height_, t.height()) ;

        labels.emplace_back(std::move(t)) ;
    }

    const double legend_gap = 4 ;

    lw = lw + legend_preview_width_ + legend_gap + legend_margin_ + legend_margin_ ;
    lh += legend_margin_ + legend_margin_ ;

    ox = w - lw - legend_padding_ ;
    oy = -h + legend_padding_ ;

    Rectangle2d fr(ox, oy, lw, lh) ;

    ox += legend_margin_ ;
    oy += legend_margin_ ;

    c.save() ;
    c.setPen(legend_bg_pen_) ;
    c.setBrush(legend_bg_brush_) ;
    c.drawRect(fr) ;
    c.restore() ;

    for( size_t i=0 ; i<nGraphs ; i++ ) {
        Text &t = labels[i] ;
        double rh = std::max(legend_min_row_height_, t.height()) ;
        c.save() ;
        c.setTransform(Matrix2d().translate(ox, oy)) ;
        graphs_[i]->drawLegend(c, legend_preview_width_, rh) ;
        c.restore() ;
        Rectangle2d pr(ox + legend_preview_width_ + legend_gap, oy, legend_max_label_width_, rh) ;
        c.save() ;
        c.setTextAlign(TextAlignLeft|TextAlignVCenter) ;
        c.setFont(legend_font_) ;
        c.setBrush(legend_label_brush_);
        c.drawText(t, pr) ;
        c.restore() ;
        oy += rh ;
    }

    /*
    if ( legend )
    {
      legw += 55 * scale ;

      qreal legh = 10 * scale + (graphList.size()) * ( maxLabelH + 5*scale ) ;

      qreal x1 = GRAPH_MARGIN_X * scale + ox, y1 = GRAPH_MARGIN_Y * scale + oy;

      switch ( lpos )
      {
        case TopLeft:
          x1 += 10*scale ;
          y1 += 10*scale ;
          break ;
        case TopCenter:
          x1 += (sx - legw)/2.0 ;
          y1 += 10*scale ;
          break ;
        case TopRight:
          x1 += sx - legw - 10*scale ;
          y1 += 10*scale ;
          break ;
        case CenterLeft:
          x1 += 10*scale ;
          y1 += (sy - legh)/2.0 ;
          break;
        case CenterRight:
          x1 += sx - legw - 10*scale ;
          y1 += (sy - legh)/2.0 ;
          break ;
        case BottomLeft:
          x1 += 10*scale ;
          y1 += sy - legh - 10*scale ;
          break ;
        case BottomCenter:
          x1 += (sx - legw)/2.0 ;
          y1 += sy - legh - 10*scale ;
          break ;
        case BottomRight:
          x1 += sx - legw - 10*scale ;
          y1 += sy - legh - 10*scale ;
          break ;
      }

      qreal x2 = x1 + legw ;
      qreal y2 = y1 + legh ;

      painter.setPen(axisp) ;
      painter.fillRect(QRect(x1, y1, x2-x1, y2-y1), QBrush(Qt::white)) ;
      painter.drawRect(x1, y1, x2-x1, y2-y1) ;

      int c = 0 ;

      for(int i=0 ; i<graphList.size() ; i++ )
      {
        LinePlot *pGraph = (LinePlot *)graphList[i] ;

        QPen gp(pGraph->lc, pGraph->lw) ;

        setDash(gp, pGraph->lstyle, scale) ;

        qreal xx = x1+5*scale ;
        qreal yy = y1 + 5*scale + c * (maxLabelH + 5*scale)  +  maxLabelH/2 ;

        if ( pGraph->lstyle != LinePlot::Empty )
        {
          painter.setPen(gp) ;
          painter.drawLine(xx, yy, xx+40*scale, yy) ;
        }

        QPen mp(pGraph->mc, pGraph->mw) ;
        QBrush mb(pGraph->mfc) ;

        DrawMarker(painter, mp, mb, QPointF(xx+20*scale, yy), pGraph->mstyle, scale,
  pGraph->msz) ;

        painter.setFont(legendFont) ;
        painter.drawText(QPointF(x1 + 50*scale, y1 + 17*scale + c * ( maxLabelH +
  5*scale )),
          pGraph->title) ;

        c++ ;
      }
    }
    */
}

}}
