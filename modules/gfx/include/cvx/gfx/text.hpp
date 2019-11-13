#ifndef CVX_GFX_TEXT_HPP
#define CVX_GFX_TEXT_HPP

#include <string>
#include <vector>

#include <cvx/gfx/canvas.hpp>
#include <cvx/gfx/font.hpp>
#include <cvx/gfx/brush.hpp>

namespace cvx { namespace gfx {


class TextSpan {
public:
    TextSpan(size_t start, size_t end): start_(start), end_(end) {}
    virtual void applyTextStyle(Canvas &canvas) const = 0;

    size_t start() const { return start_ ; }
    size_t end() const { return end_ ; }

protected:
    size_t start_, end_ ;
};

class FontSpan: public TextSpan {
public:
    FontSpan(const Font &font, size_t start, size_t end): TextSpan(start, end),  font_(font)  {}

    void applyTextStyle(Canvas &canvas) const override {
        canvas.setFont(font_) ;
    };

    Font &font() { return font_ ; }

protected:

    Font font_ ;
};

class TextFillSpan: public TextSpan {
public:
    TextFillSpan(const BrushBase &brush, size_t start, size_t stop): TextSpan(start, stop), brush_(brush.clone()) {
    }

    void applyTextStyle(Canvas &canvas) const override {
        canvas.setBrush(*brush_) ;
    }

    protected:

    std::unique_ptr<BrushBase> brush_ ;
};

struct TextSpanGroup {
    size_t start_, end_ ;
    std::vector<TextSpan *> spans_ ;
};

class StyledText {
public:

    StyledText(const std::string &txt): text_(txt) {}

    void addSpan(TextSpan *span) {
        std::unique_ptr<TextSpan> span_ptr(span) ;
        spans_.emplace_back(std::move(span_ptr)) ;
    }

    std::vector<TextSpanGroup> split() ;

protected:

    std::vector<std::unique_ptr<TextSpan>> spans_ ;
    std::string text_ ;
};





}}
#endif
