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
    btTransform local_inertial_frame_ ;
    float mass_ ;
    int mb_idx_ ;
    NodePtr node_ ;
};

struct MultiBody {
    vector<RigidBody> bodies_ ;
    vector<std::unique_ptr<btGeneric6DofSpring2Constraint>> constraints_ ;
};

void buildJoints(int link_idx, const btTransform &parent_transform_in_world_space, MultiBody &body, vector<BodyData> &links, map<string, int> &link_map, PhysicsWorld &physics) {

    btTransform link_transform_in_world_space;
    link_transform_in_world_space.setIdentity();

    BodyData &linkData = links[link_idx] ;

    const urdf::Link *link = linkData.link_, *parent_link = nullptr ;

    cout << link->name_ << endl ;
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

      btScalar mass = linkData.mass_;
      btTransform localInertialFrame = linkData.local_inertial_frame_;


      btTransform parent2joint, linkTransform;
      parent2joint.setIdentity();

      btTransform parentTransform = parent_transform_in_world_space ;

      if ( link->parent_joint_ ) {
          const urdf::Joint *j  = link->parent_joint_ ;
          parent2joint = toBulletTransform(j->origin_) ;
      }

      linkTransform = parentTransform * parent2joint;

      btTransform inertialFrameInWorldSpace = linkTransform * localInertialFrame;

      body.bodies_.emplace_back(mass, new UpdateSceneMotionState(linkData.node_), linkData.shape_, toEigenVector(linkData.inertia_)) ;

      linkData.mb_idx_ = body.bodies_.size() - 1 ;

      if ( link->parent_joint_ ) {

          BodyData &parentLinkData = links[parent_link_idx] ;

          btTransform offsetInA, offsetInB;
          offsetInA = parentLinkData.local_inertial_frame_.inverse() * parent2joint;
          offsetInB = localInertialFrame.inverse();
          btQuaternion parentRotToThis = offsetInB.getRotation() * offsetInA.inverse().getRotation();

          const urdf::Joint *j  = link->parent_joint_ ;
          btRigidBody *linkRigidBody = body.bodies_.back().handle() ;
          btRigidBody *parentRigidBody = body.bodies_[parentLinkData.mb_idx_].handle() ;

          if ( j->type_ == "fixed" ) {
             btGeneric6DofSpring2Constraint* dof6 =  new btGeneric6DofSpring2Constraint(*linkRigidBody, *parentRigidBody, offsetInB, offsetInA);
             dof6->setLinearLowerLimit(btVector3(0, 0, 0));
             dof6->setLinearUpperLimit(btVector3(0, 0, 0));

             dof6->setAngularLowerLimit(btVector3(0, 0, 0));
             dof6->setAngularUpperLimit(btVector3(0, 0, 0));

             body.constraints_.emplace_back(std::move(std::unique_ptr<btGeneric6DofSpring2Constraint>(dof6)));

          } else if ( j->type_ == "revolute" ) {
              int principleAxis = toBulletVector(j->axis_).closestAxis();

              btGeneric6DofSpring2Constraint* dof6 = nullptr ;

              switch (principleAxis)
              {
                  case 0:
                  {
                      dof6 = new btGeneric6DofSpring2Constraint(*linkRigidBody, *parentRigidBody, offsetInB, offsetInA, RO_ZYX) ;

                      dof6->setLinearLowerLimit(btVector3(0, 0, 0));
                      dof6->setLinearUpperLimit(btVector3(0, 0, 0));

                      dof6->setAngularLowerLimit(btVector3(j->lower_, 0, 0));
                      dof6->setAngularUpperLimit(btVector3(j->upper_, 0, 0));

                      break;
                  }
                  case 1:
                  {
                      dof6 = new btGeneric6DofSpring2Constraint(*linkRigidBody, *parentRigidBody, offsetInB, offsetInA, RO_XZY) ;

                      dof6->setLinearLowerLimit(btVector3(0, 0, 0));
                      dof6->setLinearUpperLimit(btVector3(0, 0, 0));

                      dof6->setAngularLowerLimit(btVector3(0, j->lower_, 0));
                      dof6->setAngularUpperLimit(btVector3(0, j->upper_, 0));
                      break;
                  }
                  case 2:
                  default:
                  {
                    dof6 = new btGeneric6DofSpring2Constraint(*linkRigidBody, *parentRigidBody, offsetInB, offsetInA, RO_XYZ) ;

                      dof6->setLinearLowerLimit(btVector3(0, 0, 0));
                      dof6->setLinearUpperLimit(btVector3(0, 0, 0));

                      dof6->setAngularLowerLimit(btVector3(0, 0, j->lower_));
                      dof6->setAngularUpperLimit(btVector3(0, 0, j->upper_));
                  }
              }

             body.constraints_.emplace_back(std::move(std::unique_ptr<btGeneric6DofSpring2Constraint>(dof6)));

          } else if ( j->type_ == "prismatic" ) {
             int principleAxis = toBulletVector(j->axis_).closestAxis();

             btGeneric6DofSpring2Constraint* dof6 = new btGeneric6DofSpring2Constraint(*linkRigidBody, *parentRigidBody, offsetInB, offsetInA, (RotateOrder)0) ;

             switch (principleAxis)
             {
                 case 0:
                 {
                     dof6->setLinearLowerLimit(btVector3(j->lower_, 0, 0));
                     dof6->setLinearUpperLimit(btVector3(j->upper_, 0, 0));
                     break;
                 }
                 case 1:
                 {
                     dof6->setLinearLowerLimit(btVector3(0, j->lower_, 0));
                     dof6->setLinearUpperLimit(btVector3(0, j->upper_, 0));
                     break;
                 }
                 case 2:
                 default:
                 {
                     dof6->setLinearLowerLimit(btVector3(0, 0, j->lower_));
                     dof6->setLinearUpperLimit(btVector3(0, 0, j->upper_));
                 }
             };

             dof6->setAngularLowerLimit(btVector3(0, 0, 0));
             dof6->setAngularUpperLimit(btVector3(0, 0, 0));

             body.constraints_.emplace_back(std::move(std::unique_ptr<btGeneric6DofSpring2Constraint>(dof6)));
          }
      }

      for( const urdf::Link *c: link->child_links_ ) {
          int child_link_idx = link_map[c->name_] ;

          buildJoints(child_link_idx, linkTransform, body, links, link_map, physics  ) ;


      }


}


MultiBody body ;

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


                 float mass = 0.0 ;
                 Isometry3f local_inertial_frame = Isometry3f::Identity() ;

                 if ( link.inertial_ ) {
                     mass = link.inertial_->mass_ ;
                     local_inertial_frame = link.inertial_->origin_ ;
                 }

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
