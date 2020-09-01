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
        TestSimulation::onUpdate(1) ;



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

void makeRobot(PhysicsWorld &physics, ScenePtr scene, const urdf::Robot &robot) {

    map<string, RigidBody> bodies ;

    for( const auto &bp: robot.links_ ) {
        const string &name = bp.first ;
        const urdf::Link &link = bp.second ;

        if ( link.collision_geom_ ) {

            urdf::Geometry *geom = link.collision_geom_.get() ;

             CollisionShape::Ptr shape ;

             if ( const urdf::BoxGeometry *g = dynamic_cast<const urdf::BoxGeometry *>(geom) ) {
                    shape.reset(new BoxCollisionShape(g->he_))  ;
             } else if ( const urdf::CylinderGeometry *g = dynamic_cast<const urdf::CylinderGeometry *>(geom) ) {
                    shape.reset(new CylinderCollisionShape(g->radius_, g->height_))  ;
             } else if ( const urdf::MeshGeometry *g = dynamic_cast<const urdf::MeshGeometry *>(geom) ) {
                    shape.reset(new StaticMeshCollisionShape(g->path_));
             }

             NodePtr node = scene->findNodeByName(link.name_);

             RigidBody body(1.0, new UpdateSceneMotionState(node), shape) ;

             physics.addBody(body) ;

             bodies.emplace(name, body) ;
        }

        for( const auto &jp: robot.joints_ ) {
            const string &name = jp.first ;
            const urdf::Joint &joint = jp.second ;



        }
    }
}

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



    DirectionalLight *dl = new DirectionalLight(Vector3f(0.5, 0.5, 1)) ;
    dl->diffuse_color_ = Vector3f(1, 1, 1) ;
    scene->addLight(LightPtr(dl)) ;

    scene->addChild(rs) ;

//    scene->matrix() = rot ;

    makeRobot(physics, scene, rb) ;

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
