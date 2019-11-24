#include <cvx/gfx/plot/legend.hpp>
#include <cvx/gfx/plot/graph.hpp>

using namespace std ;
namespace cvx { namespace gfx {

void Legend::draw(const std::vector<std::unique_ptr<Graph>> &graphs, Canvas &c, double w, double h)
{
    if ( placement_ == None ) return ;

    size_t nGraphs = graphs.size() ;

    double lw = 0, lh = 0, ox, oy ;

    vector<Text> labels ;

    for( size_t i=0 ; i<nGraphs ; i++ ) {
        Text t ;
        t.setText(graphs[i]->getTitle()) ;
        t.setWrapWidth(max_label_width_) ;
        t.setFont(label_font_) ;
        t.updateLayout() ;

        lw = std::max(lw, t.width()) ;
        lh += std::max(min_row_height_, t.height()) ;

        labels.emplace_back(std::move(t)) ;
    }

    const double legend_gap = 4 ;

    // box dimensions

    lw = lw + preview_width_ + legend_gap + margin_ + margin_ ;
    lh += margin_ + margin_ ;

    // compute offset based on placement option

    switch ( placement_ ) {
    case TopRight:
        ox = w - lw - padding_ ;
        oy = -h + padding_ ;
        break ;
    case TopLeft:
        ox = padding_ ;
        oy = -h + padding_ ;
        break ;
    case TopCenter:
        ox = (w - lw)/2.0 ;
        oy = -h + padding_ ;
        break ;
    case BottomRight:
        ox = w - lw - padding_ ;
        oy = -lh -padding_ ;
        break ;
    case BottomLeft:
        ox = padding_ ;
        oy = -lh -padding_ ;
        break ;
    case BottomCenter:
        ox = (w - lw)/2.0 ;
        oy = -lh -padding_ ;
        break ;
    case CenterRight:
        ox = w - lw - padding_ ;
        oy = -(h - lh)/2.0 ;
        break ;
    case CenterLeft:
        ox = padding_ ;
        oy =-(h - lh)/2.0 ;
        break ;

    }

    Rectangle2d fr(ox, oy, lw, lh) ;

    ox += margin_ ;
    oy += margin_ ;

    c.save() ;
    c.setPen(bg_pen_) ;
    c.setBrush(bg_brush_) ;
    c.drawRect(fr) ;
    c.restore() ;

    for( size_t i=0 ; i<nGraphs ; i++ ) {
        Text &t = labels[i] ;
        double rh = std::max(min_row_height_, t.height()) ;
        c.save() ;
        c.setTransform(Matrix2d().translate(ox, oy)) ;
        graphs[i]->drawLegend(c, preview_width_, rh) ;
        c.restore() ;
        Rectangle2d pr(ox + preview_width_ + legend_gap, oy, max_label_width_, rh) ;
        c.save() ;
        c.setTextAlign(TextAlignLeft|TextAlignVCenter) ;
        c.setFont(label_font_) ;
        c.setBrush(label_brush_);
        c.drawText(t, pr) ;
        c.restore() ;
        oy += rh ;
    }

}





}}
