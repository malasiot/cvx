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

struct BodyData {
    const urdf::Link *link_ ;
    CollisionShape::Ptr shape_ ;
    btVector3 inertia_ ;
    std::unique_ptr<btMultiBodyLinkCollider> collider_ ;
};

void buildJoints(int link_idx, Isometry3f &parent_transform_in_world_space, const vector<BodyData> &links, map<string, int> &link_map, PhysicsWorld &physics) {
    btTransform link_transform_in_world_space;
    link_transform_in_world_space.setIdentity();

    const urdf::Link *link = links[link_idx].link_ ;
    int parent_link_idx = -1 ;
    if ( link->parent_link_ ) {
        parent_link_idx = link_map[link->parent_link_->name_] ;
    }

    btRigidBody* parentRigidBody = 0;

        //b3Printf("mb link index = %d\n",mbLinkIndex);

     btTransform parentLocalInertialFrame;
     parentLocalInertialFrame.setIdentity();
     btScalar parentMass(1);
     btVector3 parentLocalInertiaDiagonal(1, 1, 1);

        btScalar mass = 0;
        btTransform localInertialFrame;
        localInertialFrame.setIdentity();
        btVector3 localInertiaDiagonal(0, 0, 0);

        btTransform parent2joint, linkTransform;
        parent2joint.setIdentity();

        btTransform parentTransform = toBulletTransform(parent_transform_in_world_space) ;

        if ( link->parent_joint_ ) {
            const urdf::Joint *j  = link->parent_joint_ ;
            parent2joint = toBulletTransform(j->origin_) ;
            linkTransform = parentTransform * parent2joint;


            btTransform offsetInA, offsetInB;
            offsetInA = parentLocalInertialFrame.inverse() * parent2joint;
            offsetInB = localInertialFrame.inverse();
            btQuaternion parentRotToThis = offsetInB.getRotation() * offsetInA.inverse().getRotation();

        }



}

void makeRobot(PhysicsWorld &physics, ScenePtr scene, const urdf::Robot &robot) {

    vector<BodyData> links ;
    map<string, int> link_map ;

    for( const auto &bp: robot.links_ ) {
        const string &name = bp.first ;
        const urdf::Link &link = bp.second ;

        if ( link.collision_geom_ ) {

            BodyData data ;

            urdf::Geometry *geom = link.collision_geom_.get() ;

             CollisionShape::Ptr shape ;

             if ( const urdf::BoxGeometry *g = dynamic_cast<const urdf::BoxGeometry *>(geom) ) {
                 shape.reset(new BoxCollisionShape(g->he_))  ;
             } else if ( const urdf::CylinderGeometry *g = dynamic_cast<const urdf::CylinderGeometry *>(geom) ) {
                 shape.reset(new CylinderCollisionShape(g->radius_, g->height_))  ;
             } else if ( const urdf::MeshGeometry *g = dynamic_cast<const urdf::MeshGeometry *>(geom) ) {
                 shape.reset(new StaticMeshCollisionShape(g->path_));
             } else if ( const urdf::SphereGeometry *g = dynamic_cast<const urdf::SphereGeometry *>(geom) ) {
                 shape.reset(new SphereCollisionShape(g->radius_));
             }

             if ( shape ) {
                 data.shape_ = shape ;
                 shape->handle()->calculateLocalInertia(1.0, data.inertia_) ;
                 data.link_ = &link ;
                 link_map[link.name_] = links.size() ;
                 links.emplace_back(std::move(data)) ;
             }
        }

         btMultiBody *body = new btMultiBody(links.size(), 0.0, {0.0, 0.0, 0.0}, true, false) ;
         body->setBaseWorldTransform(btTransform::getIdentity());



         for (int i = 0; i < body->getNumLinks(); ++i) {
            BodyData &link = links[i] ;
            btMultiBodyLinkCollider* col = new btMultiBodyLinkCollider(body, i);
            col->setCollisionShape(link.shape_->handle());
            int collisionFilterGroup = int(btBroadphaseProxy::DefaultFilter) ;
            int collisionFilterMask = int(btBroadphaseProxy::AllFilter) ;
            link.collider_.reset(col) ;
            physics.getDynamicsWorld()->addCollisionObject(col, collisionFilterGroup, collisionFilterMask);  //,2,1+2);
            body->getLink(i).m_collider = col;
        }

         int root_idx = link_map[robot.root_->name_] ;
         Eigen::Isometry3f tr = Eigen::Isometry3f::Identity() ;

         buildJoints(root_idx, tr, links, link_map, physics) ;

    }
}

void createScene() {

    physics.createDefaultDynamicsWorld();

    string package_path = "/home/malasiot/Downloads/robotiq_arg85/" ;

    Affine3f rot ;
    rot.setIdentity() ;

    rot.translate(Vector3f(0, 1.0, 0)) ;
    rot.rotate( AngleAxisf(0.5*M_PI,  Vector3f::UnitX())) ;

    string path = "/home/malasiot/local/bullet3/examples/pybullet/gym/pybullet_data/r2d2.urdf" ;
    urdf::Robot rb = urdf::Robot::load(path, /* "robots/robotiq_arg85_description.URDF",*/
    { { "robotiq_arg85_description", package_path } }, true) ;

    RobotScenePtr rs = RobotScene::fromURDF(rb) ;

    joint = std::dynamic_pointer_cast<RevoluteJoint>(rs->getJoint("left_gripper_joint")) ;



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
