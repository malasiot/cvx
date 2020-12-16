#pragma once

#include <cvx/viz/scene/scene_fwd.hpp>
#include <cvx/viz/renderer/renderer.hpp>
#include <cvx/viz/gui/simple_qt_viewer.hpp>
#include <cvx/viz/scene/camera.hpp>

#include <cvx/viz/physics/world.hpp>

#include <QOpenGLWidget>
#include <QMouseEvent>
#include <QElapsedTimer>

#include <iostream>


class SimulationGui : public cvx::viz::SimpleQtViewer
{
    Q_OBJECT
public:
    SimulationGui(cvx::viz::ScenePtr scene, cvx::viz::PhysicsWorld &physics) ;
    SimulationGui(cvx::viz::ScenePtr scene, cvx::viz::PhysicsWorld &physics, const Eigen::Vector3f &c, float r);
protected:

    cvx::viz::PhysicsWorld &physics_ ;
    cvx::viz::RayPicker picker_ ;

    bool picking_ = false ;


protected:
    void mousePressEvent(QMouseEvent *event) override;

    void mouseReleaseEvent(QMouseEvent * event) override;

    void mouseMoveEvent(QMouseEvent *event) override;

    virtual void onUpdate(float delta) override {
        physics_.stepSimulation(delta/1000.0f);
    }
};
