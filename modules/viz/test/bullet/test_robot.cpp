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

    struct Joint ;

    struct Link {
        float mass_ ;
        string name_ ;
        Link *parent_link_ = nullptr ;
        Joint *parent_joint_ = nullptr ;
        vector<Link *> child_links_ ;
        vector<Joint *> child_joints_ ;
        CollisionShape::Ptr shape_ ;
        std::unique_ptr<btMultiBodyLinkCollider> collider_ ;
        btVector3 inertia_ ;
        int mb_index_ ;
    };

    struct Joint {
        string parent_, child_ ;
        string type_ ;
        btVector3 axis_ ;
        btTransform j2p_ ;
    };

    void addLink(const string &name, float mass, CollisionShape::Ptr cshape) {
        Link l ;
        l.name_ = name ;
        l.mass_ = mass ;
        l.shape_ = cshape ;
        l.shape_->handle()->calculateLocalInertia(l.mass_, l.inertia_) ;

        links_.emplace_back(std::move(l)) ;
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

    void buildTree() {
        map<string, string> parent_link_tree ;

        for( auto &jp: joints_ ) {
            Joint &j = jp.second ;

            assert( !j.child_.empty() && !j.parent_.empty() )  ;

            int child_link_idx = findLink(j.child_) ;
            int parent_link_idx = findLink(j.parent_) ;

            assert(child_link_idx >=0 && parent_link_idx >=0) ;

            Link *child_link = &links_[child_link_idx] ;
            Link *parent_link = &links_[parent_link_idx] ;

            child_link->parent_link_ = parent_link ;
            child_link->parent_joint_ = &j ;
            parent_link->child_joints_.push_back(&j) ;
            parent_link->child_links_.push_back(child_link) ;
            parent_link_tree[child_link->name_] = j.parent_;
        }

        // find root

        root_ = nullptr ;

        for ( const auto &l: links_ ) {
            auto it = parent_link_tree.find(l.name_) ;
            if ( it == parent_link_tree.end() ) { // no parent thus root
                if ( root_ == nullptr ) {
                    int base_link_idx = findLink(l.name_) ;
                    if ( base_link_idx >=0 ) root_ = &links_[base_link_idx] ;
                }
            }
        }

        assert(root_ != nullptr) ;


        int count = 0 ;
        for( Link &l: links_ ) {
            if ( &l == root_ ) l.mb_index_ = -1 ;
            else l.mb_index_ = count++ ;
        }


    }

    void buildCollisionObject(int link_idx, const btTransform &link_transform) {
          Link &link = links_[link_idx] ;

          btMultiBodyLinkCollider* col = new btMultiBodyLinkCollider(body_.get(), link.mb_index_);

          col->setCollisionShape(link.shape_->handle());

          col->setWorldTransform(link_transform);

          link.collider_.reset(col) ;
    }

    void buildJoints(int link_idx, const btTransform &parent_transform_in_world_space) {

        btTransform link_transform_in_world_space;
        link_transform_in_world_space.setIdentity();

        Link &link = links_[link_idx] ;

        int parent_link_idx = -1 ;
        Link *parent_link = link.parent_link_ ;

        btTransform parent2joint, linkTransform;
        parent2joint.setIdentity();

        btTransform parentTransform = parent_transform_in_world_space ;
        const Joint *parent_joint  = nullptr ;

        if ( parent_link ) {
              parent_joint  = link.parent_joint_ ;
              parent2joint = parent_joint->j2p_ ;
        }

        linkTransform = parentTransform * parent2joint;

        if ( parent_joint ) {
            if ( parent_joint->type_ == "revolute" ) {
                btTransform offsetInA = parent2joint ;
                btTransform offsetInB ;
                offsetInB.setIdentity();
                btQuaternion parentRotToThis = offsetInB.getRotation() * offsetInA.inverse().getRotation();

                body_->setupRevolute(link.mb_index_, link.mass_, link.inertia_, parent_link->mb_index_,
                                     parentRotToThis, quatRotate(offsetInB.getRotation(), parent_joint->axis_), offsetInA.getOrigin(),
                                                                                                    -offsetInB.getOrigin(),
                                                                                                    true) ;
            }
        }

        buildCollisionObject(link_idx, linkTransform) ;

        for ( const Link *cl: link.child_links_ ) {
            buildJoints(findLink(cl->name_), linkTransform) ;
        }

    }

    void create(PhysicsWorld &physics)  {
        buildTree() ;

        body_.reset(new btMultiBody(links_.size()-1, root_->mass_, root_->inertia_, true, false)) ;

        btTransform tr ;
        tr.setIdentity() ;

        buildJoints(findLink(root_->name_), tr) ;



        btMultiBodyDynamicsWorld *w = static_cast<btMultiBodyDynamicsWorld *>(physics.getDynamicsWorld()) ;

        w->addMultiBody(body_.get()) ;

        for( const Link &l: links_ ) {
            //base and fixed? -> static, otherwise flag as dynamic
            bool isDynamic = (l.mb_index_ < 0 && body_->hasFixedBase()) ? false : true;
            int collisionFilterGroup = isDynamic ? int(btBroadphaseProxy::DefaultFilter) : int(btBroadphaseProxy::StaticFilter);
            int collisionFilterMask = isDynamic ? int(btBroadphaseProxy::AllFilter) : int(btBroadphaseProxy::AllFilter ^ btBroadphaseProxy::StaticFilter);

            w->addCollisionObject(l.collider_.get(), collisionFilterGroup, collisionFilterMask);

            body_->getLink(l.mb_index_).m_collider = l.collider_.get();
        }

        body_->finalizeMultiDof() ;
    }


    vector<Link> links_ ;
    map<string, int> link_map_ ;
    map<string, Joint> joints_ ;
    std::unique_ptr<btMultiBody> body_ ;
    Link *root_ ;

};


MultiBody body ;


class GUI: public TestSimulation {
public:
    GUI(cvx::viz::ScenePtr scene, cvx::viz::PhysicsWorld &physics):
    TestSimulation(scene, physics) {
    }

    void onUpdate(float delta) override {


         btTransform tr_base = body.links_[0].collider_->getWorldTransform() ;
         btTransform tr_link1 = body.links_[1].collider_->getWorldTransform() ;
         btTransform tr_link2 = body.links_[2].collider_->getWorldTransform() ;
         btTransform tr_link3 = body.links_[3].collider_->getWorldTransform() ;
         cout << toEigenTransform(tr_link3).matrix() << endl ;
         scene->findNodeByName("base")->matrix() = toEigenTransform(tr_base) ;
         scene->findNodeByName("link1")->matrix() = toEigenTransform(tr_link1) ;
         scene->findNodeByName("link2")->matrix() = toEigenTransform(tr_link2) ;
         scene->findNodeByName("link3")->matrix() = toEigenTransform(tr_link3) ;
TestSimulation::onUpdate(delta) ;
    }

};

NodePtr makeCube(const string &name, const Vector3f &hs, const Vector4f &clr, NodePtr parent) {
    PhongMaterialInstance *mat = new PhongMaterialInstance() ;
    mat->setDiffuse(clr) ;

    auto node = parent->addSimpleShapeNode(GeometryPtr(new BoxGeometry(hs)), MaterialInstancePtr(mat)) ;
    node->setName(name) ;
    return node ;
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

    body.create(physics) ;

    Isometry3f base_tr ;
    base_tr.setIdentity() ;
/*
    NodePtr base_node = makeCube(box_hs, {1, 0, 0, 1}, scene);
    NodePtr j1_node = makeJoint(j2p, base_node) ;
    NodePtr link1_node = makeCube(box_hs, {1, 1, 0, 1}, j1_node) ;
    NodePtr j2_node = makeJoint(j2p, link1_node) ;
    NodePtr link2_node = makeCube(box_hs, {1, 0, 1, 1}, j2_node) ;
    NodePtr j3_node = makeJoint(j2p, link2_node) ;
    NodePtr link3_node = makeCube(box_hs, {0, 0, 1, 1}, j3_node) ;
*/
    NodePtr base_node = makeCube("base", box_hs, {1, 0, 0, 1}, scene);

    NodePtr link1_node = makeCube("link1", box_hs, {1, 1, 0, 1}, scene) ;

    NodePtr link2_node = makeCube("link2", box_hs, {1, 0, 1, 1}, scene) ;

    NodePtr link3_node = makeCube("link3", box_hs, {0, 0, 1, 1}, scene) ;


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
