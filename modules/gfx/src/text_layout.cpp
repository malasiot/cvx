#include <cvx/gfx/text_layout.hpp>

#include "impl/text_layout_engine.hpp"
#include "impl/font_manager.hpp"

using namespace std ;

namespace cvx { namespace gfx {

Text::Text()
{

}

Text::Text(const std::string &text) {
    engine_.reset(new TextLayoutEngine(text)) ;
}

Text::~Text() {}

void Text::setText(const string &text)
{
   engine_.reset(new TextLayoutEngine(text)) ;
}

void Text::setFont(const Font &font)
{
    if ( engine_->getFont() != font ) {
        engine_->setFont(font) ;
        needs_update_ = true ;
    }
}

void Text::setLineSpacing(double ls)
{
    if ( engine_->getLineSpacing() != ls ) {
        engine_->setLineSpacing(ls) ;
        needs_update_ = true ;
    }

}

void Text::setWrapWidth(double tw) {
    if ( engine_->getWrapWidth() != tw ) {
        engine_->setWrapWidth(tw) ;
        needs_update_ = true ;
    }
}

void Text::setTextDirection(TextDirection dir) {
    if ( engine_->getTextDirection() != dir ) {
        engine_->setTextDirection(dir) ;
        needs_update_ = true ;
    }
}

void Text::updateLayout() {
    if ( needs_update_ ) {
        engine_->run() ;
        needs_update_ = false ;
    }
}

const std::vector<GlyphRun> &Text::lines() {
    updateLayout() ;
    return engine_->lines() ;
}

double Text::width() {
    updateLayout() ;
    return engine_->width() ;
}

double Text::height() {
    updateLayout() ;
    return engine_->height() ;
}

Rectangle2d Text::box(const Rectangle2d &box, uint flags)
{
    double tx = std::numeric_limits<double>::max(), ty = 0 ;

    if ( flags & TextAlignVCenter )
        ty = ( box.height() - height())/2;
    else if ( flags & TextAlignBottom )
        ty = box.height() - height()  ;

    for (const GlyphRun &line: lines() ) {
        double ox ;
        if ( flags & TextAlignHCenter )
            ox =  ( box.width() - line.width() )/2.0 ;
        else if ( flags & TextAlignRight )
            ox =  box.width() - line.width();

        tx = std::min(ox, tx) ;
    }

    return Rectangle2d(box.x() + tx, box.y() + ty, width(), height()) ;
}

}}
