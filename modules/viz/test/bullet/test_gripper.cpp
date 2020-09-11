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
#include "multi_body.hpp"

#include <QApplication>
#include <QMainWindow>

using namespace cvx::viz ;
using namespace cvx::util ;

using namespace std ;
using namespace Eigen ;

PhysicsWorld physics ;
ScenePtr scene(new Scene) ;
RNG g_rng ;

MultiBody body ;

class GUI: public TestSimulation {
public:
    GUI(cvx::viz::ScenePtr scene, cvx::viz::PhysicsWorld &physics,
         urdf::Robot &rb, const string &ctrl_joint):
    TestSimulation(scene, physics), robot_(rb), ctrl_joint_(ctrl_joint) {
        auto c = scene->geomCenter() ;
        initCamera(c, scene->geomRadius(c), SimpleQtViewer::ZAxis) ;
        joint_pos_ = 0.0 ;
    }

    void onUpdate(float delta) override {

        cout << body.getJointPosition(ctrl_joint_) << endl ;
        map<string, Isometry3f> transforms ;
        body.getLinkTransforms(transforms) ;
        scene->updateTransforms(transforms) ;
        TestSimulation::onUpdate(delta) ;


    }

    void keyPressEvent(QKeyEvent *event) override {
        joint_pos_ = body.getJointPosition(ctrl_joint_) ;
        MultiBody::Motor *m = body.getMotor(ctrl_joint_) ;

        if ( event->key() == Qt::Key_Q ) {
            joint_pos_ -= 0.1 ;
           // body.setJointPosition(ctrl_joint_, joint_pos_) ;
          //  m->setTargetVelocity(0.5) ;
            body.setJointTorque(ctrl_joint_, 0.5);
            //joint_pos_ = robot_.setJointPosition(ctrl_joint_, joint_pos_) ;
            //map<string, Isometry3f> transforms ;
           // robot_.computeLinkTransforms(transforms) ;
           // scene->updateTransforms(transforms) ;

        } else if ( event->key() == Qt::Key_W ) {
            joint_pos_ += 0.1 ;
            //body.setJointPosition(ctrl_joint_, joint_pos_) ;
            //m->setTargetVelocity(-0.5) ;
            body.setJointTorque(ctrl_joint_, -0.5);
            //joint_pos_ = robot_.setJointPosition(ctrl_joint_, joint_pos_) ;
            //map<string, Isometry3f> transforms ;
            //robot_.computeLinkTransforms(transforms) ;
            //scene->updateTransforms(transforms) ;

        }

        update() ;

    }

private:
    urdf::Robot &robot_ ;
    float joint_pos_ ;
    string ctrl_joint_ ;
};

urdf::Robot robot ;

void createScene() {

    physics.createMultiBodyDynamicsWorld();
    physics.setGravity({0, 0, -10});

    Affine3f tr(Translation3f{0, -1.5, 0}) ;

    Vector3f ground_hs{1.5f, 0.05f, 1.5f} ;
    scene->addBox(ground_hs, tr.matrix(), Vector4f{0.5, 0.5, 0.5, 1}) ;
    physics.addBody(RigidBody(CollisionShape::Ptr(new BoxCollisionShape(ground_hs)), tr)) ;


    string package_path = "/home/malasiot/Downloads/robotiq_arg85/" ;

    Affine3f rot ;
    rot.setIdentity() ;

    rot.translate(Vector3f(0, 1.0, 0)) ;
    rot.rotate( AngleAxisf(0.5*M_PI,  Vector3f::UnitX())) ;

    string path = "/home/malasiot/local/bullet3/examples/pybullet/gym/pybullet_data/cartpole.urdf" ;
    //string path = "/home/malasiot/Downloads/robotiq_arg85/" ;
    robot = urdf::Robot::load(path /*+ "robots/robotiq_arg85_description.URDF"*/,
    { { "robotiq_arg85_description", package_path } }, true) ;

    RobotScenePtr rs = RobotScene::fromURDF(robot) ;

    DirectionalLight *dl = new DirectionalLight(Vector3f(0.5, 0.5, 1)) ;
    dl->diffuse_color_ = Vector3f(1, 1, 1) ;
    scene->addLight(LightPtr(dl)) ;

    scene->addChild(rs) ;

    body.createFromURDF(physics, robot) ;

    body.setJointPosition("slider_to_cart", -1) ;
  //  body.getMotor("slider_to_cart")->setTargetVelocity(0.5) ;


}

int main(int argc, char **argv)
{
    createScene() ;

    QApplication app(argc, argv);

    cvx::viz::SimpleQtViewer::initDefaultGLContext() ;

    QMainWindow window ;
    window.setCentralWidget(new GUI(scene, physics, robot, "slider_to_cart")) ;
    window.resize(512, 512) ;
    window.show() ;

    return app.exec();
}
