#include <GL/gl3w.h>

#include <cvx/viz/renderer/renderer.hpp>
#include <cvx/viz/scene/camera.hpp>
#include <cvx/viz/scene/light.hpp>
#include <cvx/viz/scene/node.hpp>
#include <cvx/viz/scene/marker.hpp>
#include <cvx/viz/scene/material.hpp>
#include <cvx/viz/scene/geometry.hpp>

#include <cvx/util/math/rng.hpp>
#include <cvx/util/misc/strings.hpp>

#include <cvx/viz/animation/animation.hpp>

#include "test_animation.hpp"

#include <QTimer>
#include <QApplication>
#include <QMainWindow>

using namespace cvx::viz ;
using namespace cvx::util ;
using namespace Eigen ;
using namespace std ;

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

TestAnimation::TestAnimation(ScenePtr scene): scene_(scene) {


    Vector3f c{0, 0, 0};
    float r = 10.0 ;

    camera_.reset(new PerspectiveCamera(1.0, 50*M_PI/180, 0.0001, 10*r)) ;
    trackball_.setCamera(camera_, c + Vector3f{0.0, 0, 3*r}, c, {0, 1, 0}) ;
    trackball_.setZoomScale(0.1*r) ;

    camera_->setBgColor({1, 1, 1, 1}) ;

    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(updateAnimation()));
    timer->start(30);
}

void TestAnimation::mousePressEvent(QMouseEvent *event)
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

}

void TestAnimation::mouseReleaseEvent(QMouseEvent *event)
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


}

void TestAnimation::mouseMoveEvent(QMouseEvent *event)
{
    int x = event->x() ;
    int y = event->y() ;


    trackball_.setClickPoint(x, y) ;
    trackball_.update() ;

}

void TestAnimation::wheelEvent(QWheelEvent *event) {
    trackball_.setScrollDirection(event->delta()>0);
    trackball_.update() ;
    update() ;
}


NodePtr makeBox(const string &name, const Vector3f &hs, const Matrix4f &tr, const Vector4f &clr) {

    NodePtr box_node(new Node) ;
    box_node->setName(name) ;

    GeometryPtr geom(new BoxGeometry(hs)) ;

    MaterialInstancePtr material(new ConstantMaterialInstance(clr)) ;

    DrawablePtr dr(new Drawable(geom, material)) ;

    box_node->addDrawable(dr) ;

    box_node->matrix() = tr ;

    return box_node ;
}

RNG g_rng ;

ScenePtr createScene() {

    ScenePtr scene(new Scene) ;

    // create new scene and add light

    scene->addMarkerInstance(MarkerInstancePtr(new SphereMarkerInstance({0, -1, 0}, 0.2, {1, 0, 0})));

    std::shared_ptr<DirectionalLight> dl( new DirectionalLight(Vector3f(0.5, 0.5, 1)) ) ;
    dl->diffuse_color_ = Vector3f(1, 1, 1) ;
    scene->addLight(dl) ;

    scene->setPickable(true);

    {
        Affine3f tr ;
        tr.setIdentity() ;
        tr.translate(Vector3f(0, -50, 0)) ;

        NodePtr node = makeBox("ground", Vector3f{50., 50., 50.}, tr.matrix(), Vector4f{0.5, 0.5, 0.5, 1}) ;

        scene->addChild(node) ;
    }

    {
        GeometryPtr geom(new BoxGeometry(.5, .5, .5)) ;

        NodePtr node(new Node) ;

        std::shared_ptr<PhongMaterialParameters> params(new PhongMaterialParameters) ;
        params->setDiffuse({g_rng.uniform(0.0, 1.), 1, g_rng.uniform(0., 1.), 1}) ;

        MaterialInstancePtr material(new PhongMaterialInstance(params)) ;

        DrawablePtr dr(new Drawable(geom, material)) ;

        node->addDrawable(dr) ;

        node->setPickable(true);

        Affine3f tr ;
        tr.setIdentity() ;
        tr.translate(Vector3f{0., 2.0, 0.0}) ;
        node->matrix() = tr.matrix() ;

        scene->addChild(node) ;
    }

    return scene ;

}

int main(int argc, char **argv)
{

    ScenePtr scene = createScene() ;

    QApplication app(argc, argv);

    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setMajorVersion(3);
    format.setMinorVersion(3);
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    format.setSwapInterval(1);

    format.setSamples(4);
    format.setProfile(QSurfaceFormat::CoreProfile);

    QSurfaceFormat::setDefaultFormat(format);

    QMainWindow window ;
    window.setCentralWidget(new TestAnimation(scene)) ;
    window.show() ;

    return app.exec();
}
