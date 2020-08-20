#include <GL/gl3w.h>

#include "bullet_gui.hpp"

#include <QTimer>

using namespace cvx::viz ;

/*
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
*/

TestAnimation::TestAnimation(ScenePtr scene, PhysicsWorld &physics): SimpleQtViewer(), physics_(physics), picker_(physics) {
    setScene(scene) ;

    initCamera({0, 0, 0}, 10.0) ;

    camera_->setBgColor({1, 1, 1, 1}) ;

    startAnimations() ;
}

void TestAnimation::mousePressEvent(QMouseEvent *event)
{
    if ( event->modifiers() & Qt::ControlModifier ) {
        Ray ray = camera_->getRay(event->x(), event->y()) ;

        picker_.pickBody(ray);

        picking_ = true ;

    } else {
        SimpleQtViewer::mousePressEvent(event) ;
    }


}

void TestAnimation::mouseReleaseEvent(QMouseEvent *event)
{
    if ( event->modifiers() & Qt::ControlModifier ) {
        picker_.removePickingConstraint();
        picking_ = false ;
    } else {
        SimpleQtViewer::mouseReleaseEvent(event) ;
    }

}

void TestAnimation::mouseMoveEvent(QMouseEvent *event)
{
    int x = event->x() ;
    int y = event->y() ;

    if ( picking_ )  {
        Ray ray = camera_->getRay(x, y) ;
        picker_.movePickedBody(ray) ;

        update() ;
    }
    else {
        SimpleQtViewer::mouseMoveEvent(event) ;

    }

}

