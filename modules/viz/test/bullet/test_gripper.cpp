#include <cvx/viz/renderer/renderer.hpp>
#include <cvx/viz/scene/camera.hpp>
#include <cvx/viz/scene/light.hpp>
#include <cvx/viz/scene/node.hpp>
#include <cvx/viz/scene/marker.hpp>
#include <cvx/viz/scene/material.hpp>
#include <cvx/viz/scene/geometry.hpp>

#include <cvx/viz/robot/robot_scene.hpp>

#include <cvx/util/math/rng.hpp>
#include <cvx/util/misc/format.hpp>

#include <cvx/viz/physics/kinematic.hpp>

#include <iostream>
#include <thread>
#include <cvx/viz/physics/world.hpp>

#include "bullet_gui.hpp"

#include <QApplication>
#include <QMainWindow>

using namespace cvx::viz ;
using namespace cvx::util ;

using namespace std ;
using namespace Eigen ;

PhysicsWorld physics ;
ScenePtr scene(new Scene) ;
std::shared_ptr<RevoluteJoint> joint ;

RNG g_rng ;


class GUI: public TestSimulation {
public:
    GUI(cvx::viz::ScenePtr scene, cvx::viz::PhysicsWorld &physics,
        std::shared_ptr<RevoluteJoint> &joint):
    TestSimulation(scene, physics), joint_(joint) {

        joint_pos_ = 0.0 ;
    }

    void onUpdate(float delta) override {
        TestSimulation::onUpdate(delta) ;



    }

    void keyPressEvent(QKeyEvent *event) override {
        if ( event->key() == Qt::Key_Q ) {
            joint_pos_ -= 0.1 ;
            joint_pos_ = joint_->setPosition(joint_pos_) ;

        } else if ( event->key() == Qt::Key_W ) {
            joint_pos_ += 0.1 ;
            joint_pos_ = joint_->setPosition(joint_pos_) ;
        }

        update() ;

    }

private:
    std::shared_ptr<RevoluteJoint> joint_ ;
    float joint_pos_ ;
};

void createScene() {

    physics.createDefaultDynamicsWorld();

    string package_path = "/home/malasiot/Downloads/robotiq_arg85/" ;

    Affine3f rot ;
    rot.setIdentity() ;

    rot.translate(Vector3f(0, 1.0, 0)) ;
    rot.rotate( AngleAxisf(0.5*M_PI,  Vector3f::UnitX())) ;

    urdf::Robot rb = urdf::Robot::load(package_path + "robots/robotiq_arg85_description.URDF",
    { { "robotiq_arg85_description", package_path } }, true) ;

    RobotScenePtr rs = RobotScene::fromURDF(rb) ;

    joint = std::dynamic_pointer_cast<RevoluteJoint>(rs->getJoint("finger_joint")) ;

    ArticulatedCollisionShape *cs = new ArticulatedCollisionShape(rb) ;

    RigidBody abody(CollisionShape::Ptr(cs), rot) ;
    physics.addBody(abody) ;

    DirectionalLight *dl = new DirectionalLight(Vector3f(0.5, 0.5, 1)) ;
    dl->diffuse_color_ = Vector3f(1, 1, 1) ;
    scene->addLight(LightPtr(dl)) ;

    scene->addChild(rs) ;

    scene->matrix() = rot ;

        // init physics




}

int main(int argc, char **argv)
{
    createScene() ;

    QApplication app(argc, argv);

    cvx::viz::SimpleQtViewer::initDefaultGLContext() ;

    QMainWindow window ;
    window.setCentralWidget(new GUI(scene, physics, joint)) ;
    window.resize(512, 512) ;
    window.show() ;

    return app.exec();
}
