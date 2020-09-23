#pragma once

#include <cvx/gfx/surface.hpp>

#include <QtGui/QPainter>

namespace cvx { namespace gfx {

class QtSurface: public ImageSurface {
public:
    QtSurface(QPainter *p) ;

    void flush() override ;

private:

    QPainter *painter_ ;
};


}}
