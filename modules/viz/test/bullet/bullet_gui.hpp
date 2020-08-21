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


class TestSimulation : public cvx::viz::SimpleQtViewer
{
    Q_OBJECT
public:
    TestSimulation(cvx::viz::ScenePtr scene, cvx::viz::PhysicsWorld &physics) ;

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

        scene_->visit([&](cvx::viz::Node &n) {
            std::cout << n.name() << std::endl ;
            std::cout << n.matrix().matrix() << std::endl ;
        });
    }
};
