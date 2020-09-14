#include <GL/gl3w.h>

#include "bullet_gui.hpp"

#include <QTimer>

using namespace cvx::viz ;

SimulationGui::SimulationGui(ScenePtr scene, PhysicsWorld &physics): SimpleQtViewer(), physics_(physics), picker_(physics) {
    setScene(scene) ;

    auto c = scene->geomCenter();
    initCamera(c, scene->geomRadius(c)) ;

    camera_->setBgColor({1, 1, 1, 1}) ;

    startAnimations() ;
}

void SimulationGui::mousePressEvent(QMouseEvent *event)
{
    if ( event->modifiers() & Qt::ControlModifier ) {
        Ray ray = camera_->getRay(event->x(), event->y()) ;

        picker_.pickBody(ray);

        picking_ = true ;

    } else {
        SimpleQtViewer::mousePressEvent(event) ;
    }


}

void SimulationGui::mouseReleaseEvent(QMouseEvent *event)
{
    if ( event->modifiers() & Qt::ControlModifier ) {
        picker_.removePickingConstraint();
        picking_ = false ;
    } else {
        SimpleQtViewer::mouseReleaseEvent(event) ;
    }

}

void SimulationGui::mouseMoveEvent(QMouseEvent *event)
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

