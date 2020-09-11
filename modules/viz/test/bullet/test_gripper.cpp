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

        joint_pos_ = 0.0 ;
    }

    void onUpdate(float delta) override {

   //     map<string, Isometry3f> transforms ;
   //     body.getLinkTransforms(transforms) ;
   //     scene->updateTransforms(transforms) ;
         TestSimulation::onUpdate(delta) ;


    }

    void keyPressEvent(QKeyEvent *event) override {
        if ( event->key() == Qt::Key_Q ) {
            joint_pos_ -= 0.1 ;
            joint_pos_ = robot_.setJointPosition(ctrl_joint_, joint_pos_) ;
            map<string, Isometry3f> transforms ;
            robot_.computeLinkTransforms(transforms) ;
            scene->updateTransforms(transforms) ;

        } else if ( event->key() == Qt::Key_W ) {
            joint_pos_ += 0.1 ;
            joint_pos_ = robot_.setJointPosition(ctrl_joint_, joint_pos_) ;
            map<string, Isometry3f> transforms ;
            robot_.computeLinkTransforms(transforms) ;
            scene->updateTransforms(transforms) ;

        }

        update() ;

    }

private:
    urdf::Robot &robot_ ;
    float joint_pos_ ;
    string ctrl_joint_ ;
};

/*
void makeRobot(PhysicsWorld &physics, ScenePtr scene, const urdf::Robot &robot) {

    vector<BodyData> links ;
    map<string, int> link_map ;

    for( const auto &bp: robot.links_ ) {
        const string &name = bp.first ;
        const urdf::Link &link = bp.second ;

        if ( link.collision_geom_ ) {

            BodyData data ;

            urdf::Geometry *geom = link.collision_geom_.get() ;
            const Isometry3f &col_origin = geom->origin_ ;

             CollisionShape::Ptr shape ;

             if ( const urdf::BoxGeometry *g = dynamic_cast<const urdf::BoxGeometry *>(geom) ) {
                 shape.reset(new BoxCollisionShape(g->he_))  ;
             } else if ( const urdf::CylinderGeometry *g = dynamic_cast<const urdf::CylinderGeometry *>(geom) ) {
                 shape.reset(new CylinderCollisionShape(g->radius_, g->height_/2.0, CylinderCollisionShape::ZAxis))  ;
             } else if ( const urdf::MeshGeometry *g = dynamic_cast<const urdf::MeshGeometry *>(geom) ) {
                 shape.reset(new StaticMeshCollisionShape(g->path_));
             } else if ( const urdf::SphereGeometry *g = dynamic_cast<const urdf::SphereGeometry *>(geom) ) {
                 shape.reset(new SphereCollisionShape(g->radius_));
             }


             shape->handle()->setMargin(0.001) ;

             float mass = 0.0 ;
             Isometry3f local_inertial_frame = Isometry3f::Identity() ;

             if ( link.inertial_ ) {
                 mass = link.inertial_->mass_ ;
                 local_inertial_frame = link.inertial_->origin_ ;
             }

             if ( shape ) {
                GroupCollisionShape *proxy = new GroupCollisionShape() ;
                proxy->addChild(shape,  col_origin) ;
                data.shape_.reset(proxy) ;
                data.mass_ = mass ;
                data.local_inertial_frame_ = toBulletTransform(local_inertial_frame) ;
                data.link_ = &link ;
                data.node_ = scene->findNodeByName(link.name_) ;
                link_map[link.name_] = links.size() ;
                links.emplace_back(std::move(data)) ;

             }
        }
    }

    int root_idx = link_map[robot.root_->name_] ;
    btTransform tr ;
    tr.setIdentity() ;


    buildJoints(root_idx, tr, body, links, link_map, physics) ;

    for( RigidBody &b: body.bodies_ ) {
        physics.addBody(b) ;
    }

    for ( const auto &c: body.constraints_ ) {
        physics.getDynamicsWorld()->addConstraint(c.get(), true) ;
    }

    for( const auto &rp: body.motors_ ) {
        const Motor &motor = rp.second ;
        motor.constraint_->enableMotor(motor.axis_, true);
        motor.constraint_->setMaxMotorForce(motor.axis_, 0);
        motor.constraint_->setTargetVelocity(motor.axis_, 0);
    }
}
*/
urdf::Robot robot ;

void createScene() {

    physics.createMultiBodyDynamicsWorld();

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

//    scene->matrix() = rot ;

   // makeRobot(physics, scene, rb) ;



 //  body.motors_["finger_joint"].setTargetVelocity(1.5) ;




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
