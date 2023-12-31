#include <GL/gl3w.h>

#include <cvx/viz/gui/simple_qt_viewer.hpp>

#include <QTimer>

using namespace std ;
using namespace Eigen ;

namespace cvx { namespace viz {

SimpleQtViewer::SimpleQtViewer(): rdr_(Renderer::RENDER_SHADOWS) {
    setFocusPolicy(Qt::StrongFocus) ;
}

void SimpleQtViewer::initCamera(const Vector3f &c, float r, UpAxis axis) {

   axis_ = axis ;

   aradius_ = 10 * r ;

   camera_.reset(new PerspectiveCamera(1.0, 50*M_PI/180, 0.0001, 100*r)) ;
   if ( axis == YAxis )
       trackball_.setCamera(camera_, c + Vector3f{0.0, 0, 4*r}, c, {0, 1, 0}) ;
   else if ( axis == XAxis )
       trackball_.setCamera(camera_, c + Vector3f{0.0, 0, 4*r}, c, {1, 0, 0}) ;
   else if ( axis == ZAxis )
       trackball_.setCamera(camera_, c + Vector3f{0.0, 4*r, 0.0}, c, {0, 0, 1});

   trackball_.setZoomScale(0.1*r) ;

   camera_->setBgColor({1, 1, 1, 1}) ;

   rdr_.setCamera(camera_) ;
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
    GLuint fbo = this->defaultFramebufferObject() ;
    rdr_.setDefaultFBO(fbo) ;


    rdr_.render(scene_) ;

    if ( draw_axes_ ) {
        rdr_.clearZBuffer();

        rdr_.line({0, 0, 0}, {aradius_, 0, 0}, {1, 0, 0, 1}, 3);
        rdr_.line({0, 0, 0}, {0, aradius_, 0}, {0, 1, 0, 1}, 3);
        rdr_.line({0, 0, 0}, {0, 0, aradius_}, {0, 0, 1, 1}, 3);

        rdr_.text("X", Vector3f{aradius_, 0, 0}, Font("Arial", 12), Vector3f{1, 0, 0}) ;
        rdr_.text("Y", Vector3f{0, aradius_, 0}, Font("Arial", 12), Vector3f{0, 1, 0}) ;
        rdr_.text("Z", Vector3f{0, 0, aradius_}, Font("Arial", 12), Vector3f{0, 0, 1}) ;

        if ( axis_ == YAxis )
            rdr_.circle({0, 0, 0}, {0, 1, 0}, 5.0, {0, 1, 0, 1}) ;
        else if ( axis_ == XAxis )
            rdr_.circle({0, 0, 0}, {1, 0, 0}, 5.0, {0, 1, 0, 1}) ;
        else if ( axis_ == ZAxis )
            rdr_.circle({0, 0, 0}, {0, 0, 1}, 5.0, {0, 1, 0, 1}) ;
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
