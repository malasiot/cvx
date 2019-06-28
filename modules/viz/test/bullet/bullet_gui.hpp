#pragma once

#include <cvx/viz/scene/scene_fwd.hpp>
#include <cvx/viz/renderer/renderer.hpp>
#include <cvx/viz/gui/trackball.hpp>
#include <cvx/viz/scene/camera.hpp>

#include <QOpenGLWidget>
#include <QMouseEvent>
#include <QElapsedTimer>

#include "physics.hpp"

class TestAnimation : public QOpenGLWidget
{
    Q_OBJECT
public:
    TestAnimation(cvx::viz::ScenePtr scene, Physics &physics) ;

protected:

    Physics &physics_ ;
    cvx::viz::ScenePtr scene_ ;
    bool picking_ = false ;

public slots:

    void updateAnimation()  {
        onAnimationUpdate() ;
        update() ;
    }

protected:
    void mousePressEvent(QMouseEvent *event) override;

    void mouseReleaseEvent(QMouseEvent * event) override;

    void mouseMoveEvent(QMouseEvent *event) override;

    void wheelEvent ( QWheelEvent * event ) override;

    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height) override;

    virtual void onAnimationUpdate() {
        qint64 elapsed = timer_.elapsed() ;
        physics_.stepSimulation(elapsed/1000.0f);
        timer_.restart() ;
    }

    QElapsedTimer timer_ ;
    cvx::viz::Renderer rdr_ ;
    cvx::viz::CameraPtr camera_ ;
    cvx::viz::TrackBall trackball_ ;
};
