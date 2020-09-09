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

#include <bullet/BulletDynamics/Featherstone/btMultiBodyDynamicsWorld.h>

#include "bullet_gui.hpp"

#include <QApplication>
#include <QMainWindow>

using namespace cvx::viz ;
using namespace cvx::util ;

using namespace std ;
using namespace Eigen ;

PhysicsWorld physics ;
ScenePtr scene(new Scene) ;


class MultiBody {
public:
    struct Link {
        float mass_ ;
        CollisionShape::Ptr shape_ ;
        btVector3 inertia_ ;
    };

    struct Joint {
        string parent_, child_ ;
        string type_ ;
        btVector3 axis_ ;
        btTransform j2p_ ;
    };

    void addLink(const string &name, float mass, CollisionShape::Ptr cshape) {
        Link l ;
        l.mass_ = mass ;
        l.shape_ = cshape ;
        l.shape_->handle()->calculateLocalInertia(l.mass_, l.inertia_) ;
        links_.emplace_back(l) ;
        link_map_.emplace(name, links_.size()-1) ;
    }

    Joint &addRevoluteJoint(const std::string &name, string parent, string child, const Vector3f &axis, const Isometry3f &j2p) {
        Joint j ;
        j.type_ = "revolute" ;
        j.parent_ = parent  ;
        j.child_ = child ;
        j.axis_ = toBulletVector(axis) ;
        j.j2p_ = toBulletTransform(j2p) ;
        auto it = joints_.emplace(name, std::move(j)) ;
        return it.first->second ;
    }


    int findLink(const std::string &name) {
        auto it = link_map_.find(name) ;
        if ( it == link_map_.end() ) return -1 ;
        else return it->second ;
    }

    void create(const std::string &base_link)  {
        base_link_idx_ = findLink(base_link) ;
        assert(base_link_idx_ >= 0) ;

        Link &base = links_[base_link_idx_] ;
        body_.reset(new btMultiBody(links_.size()-1, base.mass_, base.inertia_, true, false)) ;

        for( const auto &jp: joints_ ) {
            const Joint &j = jp.second ;

            int parent_idx = findLink(j.parent_) ;
            int child_idx = findLink(j.child_) ;

            assert( parent_idx >= 0 && child_idx >= 0 ) ;

          //  Link &parent = links_[parent_idx] ;
            Link &child = links_[child_idx] ;

            if ( j.type_ == "revolute" ) {
                btTransform offsetInA = j.j2p_ ;
                btTransform offsetInB ;
                offsetInB.setIdentity();
                btQuaternion parentRotToThis = offsetInB.getRotation() * offsetInA.inverse().getRotation();

                body_->setupRevolute(child_idx-1, child.mass_, child.inertia_, parent_idx-1,
                                     parentRotToThis, quatRotate(offsetInB.getRotation(), j.axis_), offsetInA.getOrigin(),
                                                                                                    -offsetInB.getOrigin(),
                                                                                                    true) ;
            }
        }

        body_->finalizeMultiDof() ;
    }


    vector<Link> links_ ;
    map<string, int> link_map_ ;
    map<string, Joint> joints_ ;
    std::unique_ptr<btMultiBody> body_ ;
    int base_link_idx_ = -1;

};


MultiBody body ;


class GUI: public TestSimulation {
public:
    GUI(cvx::viz::ScenePtr scene, cvx::viz::PhysicsWorld &physics):
    TestSimulation(scene, physics) {
    }

    void onUpdate(float delta) override {

         TestSimulation::onUpdate(delta) ;

    }

};

NodePtr makeCube(const Vector3f &hs, const Vector4f &clr, NodePtr parent) {
    PhongMaterialInstance *mat = new PhongMaterialInstance() ;
    mat->setDiffuse(clr) ;

    return parent->addSimpleShapeNode(GeometryPtr(new BoxGeometry(hs)), MaterialInstancePtr(mat)) ;
}

NodePtr makeJoint(const Isometry3f &tr, NodePtr parent) {
    NodePtr j(new Node) ;
    j->matrix() = tr ;
    parent->addChild(j) ;
    return j ;
}


void createScene() {

    physics.createMultiBodyDynamicsWorld();

    Affine3f tr(Translation3f{0, -1.5, 0}) ;

    Vector3f ground_hs{3.5f, 0.05f, 3.5f} ;
    scene->addBox(ground_hs, tr.matrix(), Vector4f{0.5, 0.5, 0.5, 1}) ;
    physics.addBody(RigidBody(CollisionShape::Ptr(new BoxCollisionShape(ground_hs)), tr)) ;

    DirectionalLight *dl = new DirectionalLight(Vector3f(0.5, 0.5, 1)) ;
    dl->diffuse_color_ = Vector3f(1, 1, 1) ;
    scene->addLight(LightPtr(dl)) ;



    float link_size = 0.2 ;
    Vector3f box_hs{0.05, link_size/2, 0.05} ;
    float box_mass = 0.1 ;

    CollisionShape::Ptr box_shape(new BoxCollisionShape(box_hs)) ;

    body.addLink("base", box_mass, box_shape) ;
    body.addLink("link1", box_mass, box_shape) ;
    body.addLink("link2", box_mass, box_shape) ;
    body.addLink("link3", box_mass, box_shape) ;

    Vector3f axis = {1, 0, 0} ;
    Isometry3f j2p ;
    j2p.setIdentity() ;
    j2p.translate(Vector3f{0, -link_size, 0}) ;
    body.addRevoluteJoint("j1", "base", "link1", axis, j2p) ;
    body.addRevoluteJoint("j2", "link1", "link2", axis, j2p) ;
    body.addRevoluteJoint("j3", "link2", "link3", axis, j2p) ;

    body.create("base") ;

    static_cast<btMultiBodyDynamicsWorld *>(physics.getDynamicsWorld())->addMultiBody(body.body_.get()) ;

    Isometry3f base_tr ;
    base_tr.setIdentity() ;

    NodePtr base_node = makeCube(box_hs, {1, 0, 0, 1}, scene);
    NodePtr j1_node = makeJoint(j2p, base_node) ;
    NodePtr link1_node = makeCube(box_hs, {1, 1, 0, 1}, j1_node) ;
    NodePtr j2_node = makeJoint(j2p, link1_node) ;
    NodePtr link2_node = makeCube(box_hs, {1, 0, 1, 1}, j2_node) ;
    NodePtr j3_node = makeJoint(j2p, link2_node) ;
    NodePtr link3_node = makeCube(box_hs, {0, 0, 1, 1}, j3_node) ;

}



int main(int argc, char **argv)
{
    createScene() ;

    QApplication app(argc, argv);

    cvx::viz::SimpleQtViewer::initDefaultGLContext() ;

    QMainWindow window ;
    window.setCentralWidget(new GUI(scene, physics)) ;
    window.resize(512, 512) ;
    window.show() ;

    return app.exec();
}
