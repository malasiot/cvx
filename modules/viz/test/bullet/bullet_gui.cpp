#include <GL/gl3w.h>

#include "bullet_gui.hpp"

#include <QTimer>

using namespace cvx::viz ;

void TestAnimation::initializeGL()
{
    gl3wInit() ;
}

void TestAnimation::resizeGL(int w, int h) {
    float ratio = w/(float)h ;
    std::static_pointer_cast<PerspectiveCamera>(camera_)->setAspectRatio(ratio) ;

    trackball_.setScreenSize(w, h);
    camera_->setViewport(w, h) ;
}


void TestAnimation::paintGL()
{
    rdr_.init(camera_) ;
    rdr_.render(scene_) ;

    rdr_.clearZBuffer();

    rdr_.line({0, 0, 0}, {10, 0, 0}, {1, 0, 0, 1}, 3);
    rdr_.line({0, 0, 0}, {0, 10, 0}, {0, 1, 0, 1}, 3);
    rdr_.line({0, 0, 0}, {0, 0, 10}, {0, 0, 1, 1}, 3);

    rdr_.text("X", Vector3f{10, 0, 0}, Font("Arial", 12), Vector3f{1, 0, 0}) ;
    rdr_.text("Y", Vector3f{0, 10, 0}, Font("Arial", 12), Vector3f{0, 1, 0}) ;
    rdr_.text("Z", Vector3f{0, 0, 10}, Font("Arial", 12), Vector3f{0, 0, 1}) ;

    rdr_.circle({0, 0, 0}, {0, 1, 0}, 5.0, {0, 1, 0, 1}) ;


}

TestAnimation::TestAnimation(ScenePtr scene, Physics &physics): scene_(scene), physics_(physics) {


    Vector3f c{0, 0, 0};
    float r = 10.0 ;

    camera_.reset(new PerspectiveCamera(1.0, 50*M_PI/180, 0.0001, 10*r)) ;
    trackball_.setCamera(camera_, c + Vector3f{0.0, 0, 3*r}, c, {0, 1, 0}) ;
    trackball_.setZoomScale(0.1*r) ;

    camera_->setBgColor({1, 1, 1, 1}) ;

    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(updateAnimation()));
    timer->start(30);

    timer_.start() ;
}

void TestAnimation::mousePressEvent(QMouseEvent *event)
{
    if ( event->modifiers() & Qt::ControlModifier ) {
        Ray ray = camera_->getRay(event->x(), event->y()) ;

        physics_.pickBody(eigenVectorToBullet(ray.getOrigin()), eigenVectorToBullet(ray.getDir()*10000));

        picking_ = true ;

    } else {
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


}

void TestAnimation::mouseReleaseEvent(QMouseEvent *event)
{
    if ( event->modifiers() & Qt::ControlModifier ) {
        physics_.removePickingConstraint();
        picking_ = false ;
    } else {
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

        trackball_.update() ;
    }

}

void TestAnimation::mouseMoveEvent(QMouseEvent *event)
{
    int x = event->x() ;
    int y = event->y() ;

    if ( picking_ )  {
        Ray ray = camera_->getRay(x, y) ;

        physics_.movePickedBody(eigenVectorToBullet(ray.getOrigin()), eigenVectorToBullet(ray.getDir()*10000));
    }
    else {
        trackball_.setClickPoint(x, y) ;
        trackball_.update() ;
    }
    update() ;
}

void TestAnimation::wheelEvent(QWheelEvent *event) {
    trackball_.setScrollDirection(event->delta()>0);
    trackball_.update() ;
    update() ;
}
