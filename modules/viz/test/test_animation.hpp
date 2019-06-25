#pragma once

#include <cvx/viz/scene/scene_fwd.hpp>
#include <cvx/viz/renderer/renderer.hpp>
#include <cvx/viz/gui/trackball.hpp>
#include <cvx/viz/scene/camera.hpp>

#include <cvx/viz/animation/animation.hpp>
#include <QOpenGLWidget>
#include <QMouseEvent>
#include <QElapsedTimer>

class SimpleAnimation ;

class TestAnimation : public QOpenGLWidget
{
    Q_OBJECT

public:
    TestAnimation() ;
    virtual ~TestAnimation() override ;
protected:

    cvx::viz::ScenePtr scene_ ;
    cvx::viz::NodePtr box_ ;
    bool picking_ = false ;

public slots:

    void updateAnimation() ;

protected:
    void mousePressEvent(QMouseEvent *event) override;

    void mouseReleaseEvent(QMouseEvent * event) override;

    void mouseMoveEvent(QMouseEvent *event) override;

    void wheelEvent ( QWheelEvent * event ) override;

    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height) override;

    void createScene();

    cvx::viz::Renderer rdr_ ;
    cvx::viz::CameraPtr camera_ ;
    cvx::viz::TrackBall trackball_ ;

    QElapsedTimer et_ ;

    std::unique_ptr<SimpleAnimation> animation_ ;

};
