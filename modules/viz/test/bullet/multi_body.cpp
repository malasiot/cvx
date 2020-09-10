#include "multi_body.hpp"

#include <bullet/BulletDynamics/Featherstone/btMultiBodyDynamicsWorld.h>


using namespace cvx::viz;
using namespace std ;
using namespace Eigen ;

MultiBody::Link &MultiBody::addLink(const string &name, float mass, CollisionShape::Ptr cshape, const Isometry3f &origin) {
    Link l ;
    l.name_ = name ;
    l.mass_ = mass ;
    l.origin_ = origin ;
    l.shape_ = cshape ;
    l.shape_->handle()->calculateLocalInertia(l.mass_, l.inertia_) ;
    links_.emplace_back(std::move(l)) ;
    link_map_.emplace(name, links_.size()-1) ;
    return links_.back(); ;
}

int MultiBody::findLink(const std::string &name) {
    auto it = link_map_.find(name) ;
    if ( it == link_map_.end() ) return -1 ;
    else return it->second ;
}

void MultiBody::buildTree() {
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

void MultiBody::buildCollisionObject(int link_idx, const btTransform &link_transform) {
    Link &link = links_[link_idx] ;

    btMultiBodyLinkCollider* col = new btMultiBodyLinkCollider(body_.get(), link.mb_index_);

    btCompoundShape *proxy = new btCompoundShape() ;
    proxy->addChildShape(link.local_inertial_frame_.inverse() * toBulletTransform(link.origin_), link.shape_->handle()) ;
    link.proxy_.reset(proxy) ;
    col->setCollisionShape(proxy);

    col->setWorldTransform(link_transform);

    link.collider_.reset(col) ;
}

void MultiBody::buildJoints(int link_idx, const btTransform &parent_transform_in_world_space) {

    btTransform link_transform_in_world_space;
    link_transform_in_world_space.setIdentity();

    Link &link = links_[link_idx] ;

    Link *parent_link = link.parent_link_ ;

    btTransform parent2joint, linkTransform;
    parent2joint.setIdentity();

    btTransform parentTransform = parent_transform_in_world_space ;
    btTransform parent_local_inertial_frame ;
    parent_local_inertial_frame.setIdentity() ;
    btTransform local_inertial_frame = link.local_inertial_frame_ ;

    const Joint *parent_joint  = nullptr ;

    if ( parent_link ) {
        parent_joint  = link.parent_joint_ ;
        parent2joint = parent_joint->j2p_ ;
        parent_local_inertial_frame = parent_link->local_inertial_frame_ ;
    }

    linkTransform = parentTransform * parent2joint;

    if ( parent_joint ) {
        btTransform offsetInA = parent_local_inertial_frame.inverse() * parent2joint ;
        btTransform offsetInB = local_inertial_frame.inverse();
        btQuaternion parentRotToThis = offsetInB.getRotation() * offsetInA.inverse().getRotation();

        if ( parent_joint->type_ == RevoluteJoint  || parent_joint->type_ == ContinuousJoint ) {
            body_->setupRevolute(link.mb_index_, link.mass_, link.inertia_, parent_link->mb_index_,
                                 parentRotToThis, quatRotate(offsetInB.getRotation(), parent_joint->axis_), offsetInA.getOrigin(),
                                 -offsetInB.getOrigin(),
                                 true) ;
        } else if ( parent_joint->type_ == FixedJoint ) {
            body_->setupFixed(link.mb_index_, link.mass_, link.inertia_, parent_link->mb_index_,
                              parentRotToThis, offsetInA.getOrigin(), -offsetInB.getOrigin());
        }
    }

    buildCollisionObject(link_idx, linkTransform) ;

    for ( const Link *cl: link.child_links_ ) {
        buildJoints(findLink(cl->name_), linkTransform) ;
    }

}

void MultiBody::create(PhysicsWorld &physics)  {
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

void MultiBody::createFromURDF(PhysicsWorld &physics, urdf::Robot &rb) {
    for( const auto &lp: rb.links_ ) {
        const string &name  = lp.first ;
        const urdf::Link &link = lp.second ;

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

         float mass = 1 ;
         Isometry3f inertial_frame ;
         inertial_frame.setIdentity() ;
         Vector3f inertia(0, 0, 0) ;

         if ( link.inertial_ ) {
             mass = link.inertial_->mass_ ;
             inertial_frame = link.inertial_->origin_ ;
         }

         auto &l = addLink(name, mass, shape, col_origin) ;
         l.setLocalInertialFrame(inertial_frame) ;
    }

    for( const auto &jp: rb.joints_ ) {
        const string &name  = jp.first ;
        const urdf::Joint &joint = jp.second ;

        if ( joint.type_ == "revolute"  ) {
            auto &j = addJoint(name, RevoluteJoint, joint.parent_, joint.child_, joint.origin_) ;
            j.setAxis(joint.axis_) ;
            j.setLimits(joint.lower_, joint.upper_) ;
        } else if ( joint.type_ == "continuous" ) {
            auto &j = addJoint(name, ContinuousJoint, joint.parent_, joint.child_, joint.origin_) ;
            j.setAxis(joint.axis_) ;
        } else if ( joint.type_ == "fixed" ) {
            auto &j = addJoint(name, FixedJoint, joint.parent_, joint.child_, joint.origin_) ;
        } else if ( joint.type_ == "prismatic" ) {
            auto &j = addJoint(name, PrismaticJoint, joint.parent_, joint.child_, joint.origin_) ;
            j.setAxis(joint.axis_) ;
            j.setLimits(joint.lower_, joint.upper_) ;
        }
    }

    create(physics) ;
}

void MultiBody::getLinkTransforms(std::map<string, Isometry3f> &names) const
{
    for( const Link &l: links_ ) {
        btTransform tr = l.collider_->getWorldTransform() ;
        names.emplace(l.name_, Isometry3f(toEigenTransform(tr))) ;
    }
}

MultiBody::Joint &MultiBody::addJoint(const std::string &name, MultiBody::JointType type, const string &parent,
                                      const string &child, const Isometry3f &j2p) {
    Joint j ;
    j.type_ = type ;
    j.parent_ = parent  ;
    j.child_ = child ;
    j.j2p_ = toBulletTransform(j2p) ;

    auto it = joints_.emplace(name, std::move(j)) ;
    return it.first->second ;
}
