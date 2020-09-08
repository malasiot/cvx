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


struct Motor {
    float velocity_ = 0.0 ;
    bool enabled_ = true ;
    int axis_ = -1 ;
    btGeneric6DofSpring2Constraint *constraint_ = nullptr ;

    void setTargetVelocity(float value) {
        constraint_->setTargetVelocity(axis_, value) ;
    }
};

struct MultiBody {
    vector<RigidBody> bodies_ ;
    vector<std::unique_ptr<btGeneric6DofSpring2Constraint>> constraints_ ;
    map<std::string, Motor> motors_ ;
};

MultiBody body ;

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

      body.bodies_.emplace_back(mass, new UpdateSceneMotionState(linkData.node_), linkData.shape_) ;

      btRigidBody *linkRigidBody = body.bodies_.back().handle() ;
      linkData.mb_idx_ = body.bodies_.size() - 1 ;

      linkRigidBody->setActivationState(DISABLE_DEACTIVATION) ;

      if ( link->parent_joint_ ) {

          BodyData &parentLinkData = links[parent_link_idx] ;

          btTransform offsetInA, offsetInB;
     //     offsetInA = parentLinkData.local_inertial_frame_.inverse() * parent2joint;
          offsetInA = parent2joint;
        //  offsetInB = localInertialFrame.inverse();
          offsetInB.setIdentity() ;
        //  btQuaternion parentRotToThis = offsetInB.getRotation() * offsetInA.inverse().getRotation();

          const urdf::Joint *j  = link->parent_joint_ ;

          btRigidBody *parentRigidBody = body.bodies_[parentLinkData.mb_idx_].handle() ;

          if ( j->type_ == "fixed" ) {
             btGeneric6DofSpring2Constraint* dof6 =  new btGeneric6DofSpring2Constraint(*linkRigidBody, *parentRigidBody, offsetInB, offsetInA);


             dof6->setLinearLowerLimit(btVector3(0, 0, 0));
             dof6->setLinearUpperLimit(btVector3(0, 0, 0));

             dof6->setAngularLowerLimit(btVector3(0, 0, 0));
             dof6->setAngularUpperLimit(btVector3(0, 0, 0));

             body.constraints_.emplace_back(std::unique_ptr<btGeneric6DofSpring2Constraint>(dof6));

          } else if ( j->type_ == "revolute" || j->type_ == "continuous" ) {
              int principleAxis = toBulletVector(j->axis_).closestAxis();
              float lower = j->lower_, upper = j->upper_ ;
              if ( j->type_ == "continuous" ) {
                  lower = -1 ; upper = 1 ;
              }

              btGeneric6DofSpring2Constraint* dof6 = nullptr ;

              switch (principleAxis)
              {
                  case 0:
                  {
                      dof6 = new btGeneric6DofSpring2Constraint(*linkRigidBody, *parentRigidBody, offsetInB, offsetInA, RO_ZYX) ;

                      dof6->setLinearLowerLimit(btVector3(0, 0, 0));
                      dof6->setLinearUpperLimit(btVector3(0, 0, 0));

                      dof6->setAngularLowerLimit(btVector3(lower, 0, 0));
                      dof6->setAngularUpperLimit(btVector3(upper, 0, 0));

                      break;
                  }
                  case 1:
                  {
                      dof6 = new btGeneric6DofSpring2Constraint(*linkRigidBody, *parentRigidBody, offsetInB, offsetInA, RO_XZY) ;

                      dof6->setLinearLowerLimit(btVector3(0, 0, 0));
                      dof6->setLinearUpperLimit(btVector3(0, 0, 0));

                      dof6->setAngularLowerLimit(btVector3(0, lower, 0));
                      dof6->setAngularUpperLimit(btVector3(0, upper, 0));
                      break;
                  }
                  case 2:
                  default:
                  {
                    dof6 = new btGeneric6DofSpring2Constraint(*linkRigidBody, *parentRigidBody, offsetInB, offsetInA, RO_XYZ) ;

                      dof6->setLinearLowerLimit(btVector3(0, 0, 0));
                      dof6->setLinearUpperLimit(btVector3(0, 0, 0));

                      dof6->setAngularLowerLimit(btVector3(0, 0, lower));
                      dof6->setAngularUpperLimit(btVector3(0, 0, upper));
                  }
              }

             body.constraints_.emplace_back(std::unique_ptr<btGeneric6DofSpring2Constraint>(dof6));

             Motor motor ;


             motor.constraint_ = dof6 ;
             motor.axis_ = principleAxis ;

             body.motors_.emplace(j->name_, motor) ;

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


             body.constraints_.emplace_back(std::unique_ptr<btGeneric6DofSpring2Constraint>(dof6));


             Motor motor ;

             motor.axis_ = principleAxis ;
             motor.constraint_ = dof6 ;

            body.motors_.emplace(j->name_, motor) ;
          }
      }

      for( const urdf::Link *c: link->child_links_ ) {
          int child_link_idx = link_map[c->name_] ;

          buildJoints(child_link_idx, linkTransform, body, links, link_map, physics  ) ;


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
        motor.constraint_->setMaxMotorForce(motor.axis_, 10);
        motor.constraint_->setTargetVelocity(motor.axis_, 0);
    }
}

void createScene() {

    physics.createDefaultDynamicsWorld();

    Affine3f tr(Translation3f{0, -1.5, 0}) ;

    Vector3f ground_hs{3.5f, 0.05f, 3.5f} ;
    scene->addBox(ground_hs, tr.matrix(), Vector4f{0.5, 0.5, 0.5, 1}) ;
    physics.addBody(RigidBody(CollisionShape::Ptr(new BoxCollisionShape(ground_hs)), tr)) ;


    string package_path = "/home/malasiot/Downloads/robotiq_arg85/" ;

    Affine3f rot ;
    rot.setIdentity() ;

    rot.translate(Vector3f(0, 1.0, 0)) ;
    rot.rotate( AngleAxisf(0.5*M_PI,  Vector3f::UnitX())) ;

  //  string path = "/home/malasiot/local/bullet3/examples/pybullet/gym/pybullet_data/r2d2.urdf" ;
    string path = "/home/malasiot/Downloads/robotiq_arg85/" ;
    urdf::Robot rb = urdf::Robot::load(path + "robots/robotiq_arg85_description.URDF",
    { { "robotiq_arg85_description", package_path } }, true) ;

    RobotScenePtr rs = RobotScene::fromURDF(rb) ;

    joint = std::dynamic_pointer_cast<RevoluteJoint>(rs->getJoint("finger_joint")) ;



    DirectionalLight *dl = new DirectionalLight(Vector3f(0.5, 0.5, 1)) ;
    dl->diffuse_color_ = Vector3f(1, 1, 1) ;
    scene->addLight(LightPtr(dl)) ;

    scene->addChild(rs) ;

//    scene->matrix() = rot ;

    makeRobot(physics, scene, rb) ;



    body.motors_["finger_joint"].setTargetVelocity(1.5) ;




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
