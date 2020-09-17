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
#include <cvx/viz/physics/multi_body.hpp>

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

class GUI: public SimulationGui {
public:
    GUI(cvx::viz::ScenePtr scene, cvx::viz::PhysicsWorld &physics,
        urdf::Robot &rb, const string &ctrl_joint):
        SimulationGui(scene, physics), robot_(rb), ctrl_joint_(ctrl_joint) {

        initCamera({0, -1, 0}, 0.5, SimpleQtViewer::YAxis) ;
        vert_pos_ = 0.0, gripper_pos_ = 0.5 ;
    }

    void onUpdate(float delta) override {
        Joint *j= body.findJoint(ctrl_joint_) ;

        //       j->setMimicJointPosition();
        //       cout << body.getJointPosition(ctrl_joint_) << endl ;


        map<string, Isometry3f> transforms ;
        body.getLinkTransforms(transforms) ;
        scene->updateTransforms(transforms) ;
        SimulationGui::onUpdate(delta) ;

    }

    void keyPressEvent(QKeyEvent *event) override {

        if ( event->key() == Qt::Key_Q ) {
            vert_pos_ -= 0.05 ;
            body.setJointPosition("world_to_base", vert_pos_) ;
        } else if ( event->key() == Qt::Key_W ) {
            vert_pos_ += 0.05 ;
            body.setJointPosition("world_to_base", vert_pos_) ;
        } else if ( event->key() == Qt::Key_A ) {
            gripper_pos_ -= 0.03 ;
            body.setJointPosition("left_gripper_joint", gripper_pos_) ;
            body.setJointPosition("right_gripper_joint", gripper_pos_) ;
        }  else if ( event->key() == Qt::Key_S ) {
            gripper_pos_ += 0.03 ;
            body.setJointPosition("left_gripper_joint", gripper_pos_) ;
            body.setJointPosition("right_gripper_joint", gripper_pos_) ;
        }

        update() ;

    }

private:
    urdf::Robot &robot_ ;
    float vert_pos_ = 0.0, gripper_pos_ = 0.5 ;
    string ctrl_joint_ ;
};

urdf::Robot robot ;

void createScene() {

    physics.createMultiBodyDynamicsWorld();
    physics.setGravity({0, -10, 0});

    Affine3f tr(Translation3f{0, -1.5, 0}) ;

    Vector3f ground_hs{1.5f, 0.05f, 1.5f} ;
    scene->addBox(ground_hs, tr.matrix(), Vector4f{0.5, 0.5, 0.5, 1}) ;
    physics.addBody(RigidBody(CollisionShape::Ptr(new BoxCollisionShape(ground_hs)), tr)) ;

    Affine3f box_tr(Translation3f{0, -1.2, 0}) ;
    Vector3f box_hs{0.03, 0.03f, 0.03f} ;
    auto box = scene->addBox(box_hs, box_tr.matrix(), Vector4f{0.5, 1.5, 0, 1}) ;
    physics.addBody(RigidBody(2.0, new UpdateSceneMotionState(box), CollisionShape::Ptr(new BoxCollisionShape(box_hs)))) ;


    string path = "/home/malasiot/local/bullet3/examples/pybullet/gym/pybullet_data/pr2_gripper.urdf" ;
    robot = urdf::Robot::load(path, { }, true) ;

    RobotScenePtr rs = RobotScene::fromURDF(robot) ;

    DirectionalLight *dl = new DirectionalLight(Vector3f(0.5, 0.5, 1)) ;
    dl->diffuse_color_ = Vector3f(1, 1, 1) ;
    scene->addLight(LightPtr(dl)) ;

    scene->addChild(rs) ;

    Isometry3f global = Isometry3f::Identity() ;
    global.rotate(AngleAxisf(-M_PI/2, Vector3f::UnitZ())) ;

    body.loadURDF(robot) ;

    body.addLink("world", 0.0, nullptr) ;
    body.addJoint("world_to_base", PrismaticJoint, "world", "gripper_pole", global).setAxis(Vector3f::UnitX()) ;
    body.create(physics) ;




}

int main(int argc, char **argv)
{
    createScene() ;

    QApplication app(argc, argv);

    SimpleQtViewer::initDefaultGLContext() ;

    QMainWindow window ;
    window.setCentralWidget(new GUI(scene, physics, robot, "left_gripper_joint")) ;
    window.resize(512, 512) ;
    window.show() ;

    return app.exec();
}
