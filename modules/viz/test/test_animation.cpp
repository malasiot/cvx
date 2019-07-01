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
#include <QElapsedTimer>

#include <Eigen/Geometry>

using namespace cvx::viz ;
using namespace cvx::util ;
using namespace Eigen ;
using namespace std ;


class SimpleAnimation: public Animation {
public:
    SimpleAnimation(): Animation() {
        setDuration(2500) ;
       // setRepeatCount(0) ;
       // setRepeatMode(Animation::RESTART) ;

        translation_sampler_.reset(new LinearKeyFrameSampler<Vector3f>()) ;
        translation_tl_.reset(new TimeLine<Vector3f>()) ;
        translation_tl_->addKeyFrame(0.0, {0.0f, 2.0f, 1.0f}) ;
        translation_tl_->addKeyFrame(1.0, {4.0f, 2.0f, 1.0f}) ;
        translation_channel_.reset(new TimeLineChannel<Vector3f>(*translation_tl_, translation_sampler_.get(), &lec)) ;
        addChannel(translation_channel_.get()) ;

        rotation_sampler_.reset(new LinearKeyFrameSampler<Quaternionf>()) ;
        rotation_tl_.reset(new TimeLine<Quaternionf>()) ;
        rotation_tl_->addKeyFrame(0.0, {1.0, 0.0f, 0.0f, 0.0f}) ;
        rotation_tl_->addKeyFrame(1.0, {1.0f, 0.5f, 0.0f, 0.0f}) ;
        rotation_channel_.reset(new TimeLineChannel<Quaternionf>(*rotation_tl_, rotation_sampler_.get(), &lec)) ;
        addChannel(rotation_channel_.get()) ;
    }

    std::unique_ptr<TimeLine<Vector3f>> translation_tl_ ;
    std::unique_ptr<KeyFrameSampler<Vector3f>> translation_sampler_ ;
    std::unique_ptr<TimeLineChannel<Vector3f>> translation_channel_ ;

    std::unique_ptr<TimeLine<Quaternionf>> rotation_tl_ ;
    std::unique_ptr<KeyFrameSampler<Quaternionf>> rotation_sampler_ ;
    std::unique_ptr<TimeLineChannel<Quaternionf>> rotation_channel_ ;

    EaseInOutCubic lec ;
};

TestAnimation::~TestAnimation() = default ;

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

TestAnimation::TestAnimation(): rdr_(Renderer::RENDER_SHADOWS) {

    createScene() ;

    Vector3f c = scene_->geomCenter() ;
    float r = scene_->geomRadius(c) ;

    camera_.reset(new PerspectiveCamera(1.0, 50*M_PI/180, 0.0001, 100*r)) ;
    trackball_.setCamera(camera_, c + Vector3f{0.0, 0, 4*r}, c, {0, 1, 0}) ;
    trackball_.setZoomScale(0.1*r) ;

    camera_->setBgColor({1, 1, 1, 1}) ;

    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(updateAnimation()));

    et_.start() ;
    timer->start(30);

    animation_.reset(new SimpleAnimation()) ;
    animation_->start((float)et_.elapsed()) ;

    scene_->startAnimations((float)et_.elapsed());
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
         trackball_.update() ;

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
 trackball_.setClickPoint(event->x(), event->y()) ;
  trackball_.update() ;

}

void TestAnimation::mouseMoveEvent(QMouseEvent *event)
{
    int x = event->x() ;
    int y = event->y() ;


    trackball_.setClickPoint(x, y) ;
    trackball_.update() ;

    update() ;

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

void TestAnimation::createScene() {

    scene_.reset(new Scene) ;

    scene_->load("/home/malasiot/Downloads/human.fbx", Scene::IMPORT_SKELETONS | Scene::IMPORT_ANIMATIONS) ;

    // create new scene and add light

    std::shared_ptr<DirectionalLight> dl( new DirectionalLight(Vector3f(0.5, 0.5, 1)) ) ;
    dl->diffuse_color_ = Vector3f(1, 1, 1) ;
    scene_->addLight(dl) ;

    scene_->setPickable(true);

    box_ = scene_ ;

    {
        Affine3f tr ;
        tr.setIdentity() ;
        tr.translate(Vector3f(0, 0, 0)) ;

        NodePtr node = makeBox("ground", Vector3f{50., 0., 50.}, tr.matrix(), Vector4f{0.5, 0.5, 0.5, 1}) ;

        scene_->addChild(node) ;
    }
 return ;
    {
        GeometryPtr geom(new BoxGeometry(.5, .5, .5)) ;

        NodePtr node(new Node) ;


        PhongMaterialInstance *material(new PhongMaterialInstance()) ;
        material->setDiffuse({g_rng.uniform(0.0, 1.), 1, g_rng.uniform(0., 1.), 1}) ;

        DrawablePtr dr(new Drawable(geom, std::shared_ptr<MaterialInstance>(material))) ;

        node->addDrawable(dr) ;

        node->setPickable(true);

        Affine3f tr ;
        tr.setIdentity() ;
        tr.translate(Vector3f{0., 2.0, 0.0}) ;
        node->matrix() = tr.matrix() ;

        scene_->addChild(node) ;

        box_ = node ;
    }

}

void TestAnimation::updateAnimation() {

    scene_->updateAnimations((float)et_.elapsed()) ;
    /*
    const TimeLineChannel<Vector3f> *translation_channel = animation_->translation_channel_.get() ;
    const TimeLineChannel<Quaternionf> *rotation_channel = animation_->rotation_channel_.get() ;

    animation_->update((float)et_.elapsed()) ;
    Vector3f tval = translation_channel->getValue() ;
    Quaternionf rval = rotation_channel->getValue() ;

    Affine3f &mat = box_->matrix() ;

    mat.translation() = tval ;

    mat.linear() = rval.toRotationMatrix() ;
*/
    update() ;


}

int main(int argc, char **argv)
{



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
    window.setCentralWidget(new TestAnimation()) ;
    window.resize(512, 512) ;
    window.show() ;

    return app.exec();
}
