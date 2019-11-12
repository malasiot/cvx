#ifndef CVX_GFX_GLYPH_HPP
#define CVX_GFX_GLYPH_HPP

#include <string>
#include <vector>

namespace cvx { namespace gfx {

struct Glyph {

    Glyph(unsigned cp): index_(cp) {}

    unsigned index_;  // glyph code point
    double x_advance_, y_advance_;  // amount to advance cursor
    double x_offset_, y_offset_ ;   // glyphs offset
 };


} }

#endif
