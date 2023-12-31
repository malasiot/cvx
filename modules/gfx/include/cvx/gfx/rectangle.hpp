#ifndef CVX_GFX_RECTANGLE_HPP
#define CVX_GFX_RECTANGLE_HPP

#include <cvx/gfx/vector.hpp>

namespace cvx { namespace gfx {

class Rectangle2d
{
protected:

    double x_, y_, width_ = 0, height_ = 0 ;
    bool empty_ ;

public:

    Rectangle2d(): empty_(true) {}

    Rectangle2d(double x, double y, double w, double h): x_(x), y_(y), width_(w), height_(h), empty_(false) {
        normalize() ;
    }

    Rectangle2d(const Vector2d &tl, const Vector2d &br): empty_(false) {
        x_ = tl.x() ; y_ = tl.y() ;
        width_ = br.x() - x_ ;
        height_ = br.y() - y_ ;
    }

    Point2d topLeft() const { return { x_, y_ } ; }
    Point2d bottomRight() const { return { x_ + width_, y_ + height_ } ; }
    Point2d bottomLeft() const { return { x_, y_ + height_ } ; }
    Point2d topRight() const { return { x_ + width_, y_ } ; }
    Point2d center() const { return { x_ + width_/2, y_ + height_/2} ; }

    double width() const { return width_ ; }
    double height() const { return height_ ; }
    double &width() { return width_ ; }
    double &height() { return height_ ; }

    double x() const { return x_ ; }
    double y() const { return y_ ; }
    double &x() { return x_ ; }
    double &y() { return y_ ; }

    bool empty() const { return empty_ ; }

    void extend(const Point2d &p) {
        if ( empty_ ) {
            x_ = p.x() ; y_ = p.y() ;
            empty_ = false ;
        } else {
            double tlx = std::min(x_, p.x()) ;
            double tly = std::min(y_, p.y()) ;
            double brx = std::max(x_ + width_, p.x()) ;
            double bry = std::max(y_ + width_, p.y()) ;

            x_ = tlx ; y_ = tly ;
            width_ = brx - tlx ; height_ = bry - tly ;
        }
    }


private:

    void normalize() {
        double tlx = std::min(x_, x_ + width_) ;
        double tly = std::min(y_, y_ + height_) ;
        double brx = std::max(x_, x_ + width_) ;
        double bry = std::max(y_, y_ + height_) ;

        x_ = tlx ; y_ = tly ;
        width_ = brx - tlx ; height_ = bry - tly ;
    }

};


}}
#endif
