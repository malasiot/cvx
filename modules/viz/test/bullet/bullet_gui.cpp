#include <GL/gl3w.h>

#include "bullet_gui.hpp"

#include <QTimer>

using namespace cvx::viz ;

TestSimulation::TestSimulation(ScenePtr scene, PhysicsWorld &physics): SimpleQtViewer(), physics_(physics), picker_(physics) {
    setScene(scene) ;

    auto c = scene->geomCenter();
    initCamera(c, scene->geomRadius(c), {0, 1, 0}) ;

    camera_->setBgColor({1, 1, 1, 1}) ;

    startAnimations() ;
}

void TestSimulation::mousePressEvent(QMouseEvent *event)
{
    if ( event->modifiers() & Qt::ControlModifier ) {
        Ray ray = camera_->getRay(event->x(), event->y()) ;

        picker_.pickBody(ray);

        picking_ = true ;

    } else {
        SimpleQtViewer::mousePressEvent(event) ;
    }


}

void TestSimulation::mouseReleaseEvent(QMouseEvent *event)
{
    if ( event->modifiers() & Qt::ControlModifier ) {
        picker_.removePickingConstraint();
        picking_ = false ;
    } else {
        SimpleQtViewer::mouseReleaseEvent(event) ;
    }

}

void TestSimulation::mouseMoveEvent(QMouseEvent *event)
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

