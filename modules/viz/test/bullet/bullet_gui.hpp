#pragma once

#include <cvx/viz/scene/scene_fwd.hpp>
#include <cvx/viz/renderer/renderer.hpp>
#include <cvx/viz/gui/simple_qt_viewer.hpp>
#include <cvx/viz/scene/camera.hpp>

#include <QOpenGLWidget>
#include <QMouseEvent>
#include <QElapsedTimer>

#include "physics.hpp"

class TestAnimation : public cvx::viz::SimpleQtViewer
{
    Q_OBJECT
public:
    TestAnimation(cvx::viz::ScenePtr scene, Physics &physics) ;

protected:

    Physics &physics_ ;
    bool picking_ = false ;


protected:
    void mousePressEvent(QMouseEvent *event) override;

    void mouseReleaseEvent(QMouseEvent * event) override;

    void mouseMoveEvent(QMouseEvent *event) override;

    virtual void onUpdate(float delta) override {
        physics_.stepSimulation(delta/1000.0f);
    }


};
