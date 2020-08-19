#include <GL/gl3w.h>

#include <cvx/viz/gui/simple_qt_viewer.hpp>

#include <QTimer>

using namespace std ;
using namespace Eigen ;

namespace cvx { namespace viz {

SimpleQtViewer::SimpleQtViewer()
{
    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setMajorVersion(3);
    format.setMinorVersion(3);
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    format.setSwapInterval(1);

    format.setSamples(4);
    format.setProfile(QSurfaceFormat::CoreProfile);

    QSurfaceFormat::setDefaultFormat(format);
}

void SimpleQtViewer::initCamera(const Vector3f &c, float r) {

   camera_.reset(new PerspectiveCamera(1.0, 50*M_PI/180, 0.0001, 100*r)) ;
   trackball_.setCamera(camera_, c + Vector3f{0.0, 0, 4*r}, c, {0, 1, 0}) ;
   trackball_.setZoomScale(0.1*r) ;

   camera_->setBgColor({1, 1, 1, 1}) ;
}

void SimpleQtViewer::setScene(const ScenePtr &scene) {
    scene_ = scene ;
}

void SimpleQtViewer::startAnimations()
{
    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(updateAnimation()));

    et_.start() ;
    timer->start(30);
}

void SimpleQtViewer::mousePressEvent(QMouseEvent *event)
{
    switch ( event->button() ) {
    case Qt::LeftButton:
        trackball_.setLeftClicked(true) ;
        break ;
    case Qt::MiddleButton:
        trackball_.setMiddleClicked(true) ;
        break ;
    case Qt::RightButton:
        trackball_.setRightClicked(true) ;
        break ;
    }
    trackball_.setClickPoint(event->x(), event->y()) ;
    trackball_.update() ;
}

void SimpleQtViewer::mouseReleaseEvent(QMouseEvent *event)
{
    switch ( event->button() ) {
    case Qt::LeftButton:
        trackball_.setLeftClicked(false) ;
        break ;
    case Qt::MiddleButton:
        trackball_.setMiddleClicked(false) ;
        break ;
    case Qt::RightButton:
        trackball_.setRightClicked(false) ;
        break ;
    }
    trackball_.setClickPoint(event->x(), event->y()) ;
    trackball_.update() ;

}

void SimpleQtViewer::mouseMoveEvent(QMouseEvent *event)
{
    int x = event->x() ;
    int y = event->y() ;

    trackball_.setClickPoint(x, y) ;
    trackball_.update() ;

    update() ;
}

void SimpleQtViewer::wheelEvent(QWheelEvent *event) {
    trackball_.setScrollDirection(event->delta()>0);
    trackball_.update() ;
    update() ;
}


void SimpleQtViewer::initializeGL()
{
    gl3wInit() ;
}

void SimpleQtViewer::resizeGL(int w, int h) {
    float ratio = w/(float)h ;
    std::static_pointer_cast<PerspectiveCamera>(camera_)->setAspectRatio(ratio) ;

    trackball_.setScreenSize(w, h);
    camera_->setViewport(w, h) ;
}


void SimpleQtViewer::paintGL()
{
    rdr_.init(camera_) ;
    rdr_.render(scene_) ;

    if ( draw_axes_ ) {
        rdr_.clearZBuffer();

        rdr_.line({0, 0, 0}, {10, 0, 0}, {1, 0, 0, 1}, 3);
        rdr_.line({0, 0, 0}, {0, 10, 0}, {0, 1, 0, 1}, 3);
        rdr_.line({0, 0, 0}, {0, 0, 10}, {0, 0, 1, 1}, 3);

        rdr_.text("X", Vector3f{10, 0, 0}, Font("Arial", 12), Vector3f{1, 0, 0}) ;
        rdr_.text("Y", Vector3f{0, 10, 0}, Font("Arial", 12), Vector3f{0, 1, 0}) ;
        rdr_.text("Z", Vector3f{0, 0, 10}, Font("Arial", 12), Vector3f{0, 0, 1}) ;

        rdr_.circle({0, 0, 0}, {0, 1, 0}, 5.0, {0, 1, 0, 1}) ;
    }
}

void SimpleQtViewer::updateAnimation() {

    float elapsed = et_.elapsed() ;

    if ( anim_cb_ != nullptr )
        anim_cb_(elapsed) ;
    else
        onUpdate(elapsed) ;

    et_.restart() ;

    update() ;

}

}}
