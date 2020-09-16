#include <cvx/viz/physics/multi_body.hpp>

#include <bullet/BulletDynamics/Featherstone/btMultiBodyDynamicsWorld.h>
#include <bullet/BulletDynamics/Featherstone/btMultiBodyJointLimitConstraint.h>


using namespace std ;
using namespace Eigen ;

namespace cvx { namespace viz {


Link &MultiBody::addLink(const string &name, float mass, CollisionShape::Ptr cshape, const Isometry3f &origin) {
    Link l ;
    l.name_ = name ;
    l.mass_ = mass ;
    l.origin_ = origin ;
    l.shape_ = cshape ;
    if ( cshape ) {
        l.shape_->handle()->calculateLocalInertia(l.mass_, l.inertia_) ;
    }
    links_.emplace_back(std::move(l)) ;
    link_map_.emplace(name, links_.size()-1) ;
    return links_.back(); ;
}

int MultiBody::findLink(const std::string &name) {
    auto it = link_map_.find(name) ;
    if ( it == link_map_.end() ) return -1 ;
    else return it->second ;
}

Joint *MultiBody::findJoint(const string &name) {
    auto it = joints_.find(name) ;
    if ( it == joints_.end() ) return nullptr ;
    else return &(it->second) ;
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

        j.parent_link_ = parent_link ;
        j.child_link_ = child_link ;

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

    if ( link.shape_ ) {
        btMultiBodyLinkCollider* col = new btMultiBodyLinkCollider(body_.get(), link.mb_index_);

        btCompoundShape *proxy = new btCompoundShape() ;
        proxy->addChildShape(link.local_inertial_frame_.inverse() * toBulletTransform(link.origin_), link.shape_->handle()) ;
        link.proxy_.reset(proxy) ;
        col->setCollisionShape(proxy);

        col->setWorldTransform(link_transform);

        link.collider_.reset(col) ;
    }
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

    Joint *parent_joint  = nullptr ;

    if ( parent_link ) {
        parent_joint  = link.parent_joint_ ;
        parent2joint = parent_joint->j2p_ ;
        parent_local_inertial_frame = parent_link->local_inertial_frame_ ;
    }

    linkTransform = parentTransform * parent2joint  ;

    if ( parent_joint ) {
        parent_joint->body_ = body_.get() ;

        btMultibodyLink &mb_link = body_->getLink(link.mb_index_) ;
        mb_link.m_jointDamping = parent_joint->damping_;
        mb_link.m_jointFriction = parent_joint->friction_;
        mb_link.m_jointLowerLimit = parent_joint->lower_;
        mb_link.m_jointUpperLimit = parent_joint->upper_;
        mb_link.m_jointMaxForce = parent_joint->max_force_;
        mb_link.m_jointMaxVelocity = parent_joint->max_velocity_;

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
        } else if ( parent_joint->type_ == PrismaticJoint ) {
            body_->setupPrismatic(link.mb_index_, link.mass_, link.inertia_, parent_link->mb_index_,
                                                                            parentRotToThis, quatRotate(offsetInB.getRotation(), parent_joint->axis_), offsetInA.getOrigin(),
                                                                            -offsetInB.getOrigin(),
                                                                            true);

            if ( parent_joint->lower_ <= parent_joint->upper_ ) {
                btMultiBodyConstraint* con = new btMultiBodyJointLimitConstraint(body_.get(), link.mb_index_, parent_joint->lower_, parent_joint->upper_);
                constraints_.emplace_back(unique_ptr<btMultiBodyConstraint>(con)) ;
            }

        } else if ( parent_joint->type_ == SphericalJoint ) {
            body_->setupSpherical(link.mb_index_, link.mass_, link.inertia_, parent_link->mb_index_,
                                        parentRotToThis, offsetInA.getOrigin(), -offsetInB.getOrigin(),
                                        true);
        } else if ( parent_joint->type_ == PlanarJoint ) {
           body_->setupPlanar(link.mb_index_, link.mass_, link.inertia_, parent_link->mb_index_,
                                        parentRotToThis, quatRotate(offsetInB.getRotation(), parent_joint->axis_), offsetInA.getOrigin(),
                                        true);
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
        if ( l.collider_ == nullptr ) continue ;
        //base and fixed? -> static, otherwise flag as dynamic
        bool isDynamic = (l.mb_index_ < 0 && body_->hasFixedBase()) ? false : true;
        int collisionFilterGroup = isDynamic ? int(btBroadphaseProxy::DefaultFilter) : int(btBroadphaseProxy::StaticFilter);
        int collisionFilterMask = isDynamic ? int(btBroadphaseProxy::AllFilter) : int(btBroadphaseProxy::AllFilter ^ btBroadphaseProxy::StaticFilter);

        w->addCollisionObject(l.collider_.get(), collisionFilterGroup, collisionFilterMask);

        body_->getLink(l.mb_index_).m_collider = l.collider_.get();
    }

    // create motors

    // mb->setBaseName(name->c_str());
    //create motors for each btMultiBody joint

    for( const Link &l: links_ ) {
        int mb_link_idx = l.mb_index_ ;
        if ( mb_link_idx < 0 ) continue ;

        btMultibodyLink &mb_link = body_->getLink(mb_link_idx) ;

        mb_link.m_linkName = l.name_.c_str() ;
        mb_link.m_jointName = l.parent_joint_->name_.c_str();

        if ( mb_link.m_jointType == btMultibodyLink::eRevolute || mb_link.m_jointType == btMultibodyLink::ePrismatic)
        {
            btMultiBodyJointMotor *motor = new btMultiBodyJointMotor(body_.get(), mb_link_idx, 0, 0, l.parent_joint_->motor_max_force_);
            l.parent_joint_->motor_ = motor;
            motor->setVelocityTarget(0) ;
            constraints_.emplace_back(unique_ptr<btMultiBodyConstraint>(motor)) ;
        }
    }

    for( const auto &c: constraints_ ) {
        c.get()->finalizeMultiDof();
        w->addMultiBodyConstraint(c.get()) ;
    }

    body_->finalizeMultiDof() ;
}

void MultiBody::setMimic(const urdf::Joint &joint, Joint &j) {
    if ( !joint.mimic_joint_.empty() ) {
        Joint *mj = findJoint(joint.mimic_joint_) ;
        if ( mj ) {
            mj->mimic_joints_.push_back(&j) ;
            j.mimic_multiplier_ = joint.mimic_multiplier_ ;
            j.mimic_offset_ = joint.mimic_offset_ ;
        }
    }
}

void MultiBody::createFromURDF(PhysicsWorld &physics, urdf::Robot &rb) {
    for( const auto &lp: rb.links_ ) {
        const string &name  = lp.first ;
        const urdf::Link &link = lp.second ;

        urdf::Geometry *geom = link.collision_geom_.get() ;
        Isometry3f col_origin = Isometry3f::Identity();
        if ( geom ) col_origin = geom->origin_ ;

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

         if ( shape )
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
            j.setDamping(joint.damping_) ;
            j.setFriction(joint.friction_) ;
            j.setMaxForce(joint.effort_) ;
            j.setMaxVelocity(joint.velocity_) ;
            setMimic(joint, j) ;
        } else if ( joint.type_ == "continuous" ) {
            auto &j = addJoint(name, ContinuousJoint, joint.parent_, joint.child_, joint.origin_) ;
            j.setAxis(joint.axis_) ;
            j.setDamping(joint.damping_) ;
            j.setFriction(joint.friction_) ;
            j.setMaxForce(joint.effort_) ;
            j.setMaxVelocity(joint.velocity_) ;
            setMimic(joint, j) ;
        } else if ( joint.type_ == "fixed" ) {
            auto &j = addJoint(name, FixedJoint, joint.parent_, joint.child_, joint.origin_) ;
        } else if ( joint.type_ == "prismatic" ) {
            auto &j = addJoint(name, PrismaticJoint, joint.parent_, joint.child_, joint.origin_) ;
            j.setAxis(joint.axis_) ;
            j.setLimits(joint.lower_, joint.upper_) ;
            j.setDamping(joint.damping_) ;
            j.setFriction(joint.friction_) ;
            j.setMaxForce(joint.effort_) ;
            j.setMaxVelocity(joint.velocity_) ;
            setMimic(joint, j) ;
        } else if ( joint.type_ == "planar" ) {
            auto &j = addJoint(name, PlanarJoint, joint.parent_, joint.child_, joint.origin_) ;
            j.setAxis(joint.axis_) ;
            j.setDamping(joint.damping_) ;
            j.setFriction(joint.friction_) ;
            j.setMaxForce(joint.effort_) ;
            j.setMaxVelocity(joint.velocity_) ;
        } else if ( joint.type_ == "floating" ) {
            assert("unsupported") ;
        }
    }

    create(physics) ;
}

void MultiBody::getLinkTransforms(std::map<string, Isometry3f> &names) const
{
    for( const Link &l: links_ ) {
        if ( l.collider_ ) {
            btTransform tr = l.collider_->getWorldTransform();
            names.emplace(l.name_, Isometry3f(toEigenTransform(tr))) ;
        }
    }
}

Joint &MultiBody::addJoint(const std::string &name, JointType type, const string &parent,
                                      const string &child, const Isometry3f &j2p) {
    Joint j ;
    j.name_ = name ;
    j.type_ = type ;
    j.parent_ = parent  ;
    j.child_ = child ;
    j.j2p_ = toBulletTransform(j2p) ;

    auto it = joints_.emplace(name, std::move(j)) ;
    return it.first->second ;
}

int MultiBody::getJointIndex(const string &name) {
    cout << name << endl ;
    Joint *j = findJoint(name) ;
    assert( j != nullptr ) ;
    return j->childLink()->mb_index_ ;
}

float MultiBody::getJointPosition(const string &name)
{
    return static_cast<float>(body_->getJointPos(getJointIndex(name))) ;
}

float MultiBody::getJointVelocity(const string &name) {
    return static_cast<float>(body_->getJointVel(getJointIndex(name))) ;
}

float MultiBody::getJointTorque(const string &name) {
    return static_cast<float>(body_->getJointTorque(getJointIndex(name))) ;
}

void MultiBody::setJointPosition(const string &name, float v) {
    body_->setJointPos(getJointIndex(name), static_cast<btScalar>(v)) ;
}

void MultiBody::setJointVelocity(const string &name, float v) {
    body_->setJointVel(getJointIndex(name), static_cast<btScalar>(v)) ;
}

void MultiBody::setJointTorque(const string &name, float v) {
    body_->addJointTorque(getJointIndex(name), static_cast<btScalar>(v)) ;
}


void Joint::setTargetVelocity(float v) {
    motor_->setVelocityTarget(static_cast<btScalar>(v)) ;
}

void Joint::setTargetPosition(float v) {
    motor_->setPositionTarget(static_cast<btScalar>(v));
}

Joint & Joint::setMotorMaxImpulse(float v) {
    motor_max_force_ = v ;
    return *this ;
}

void Joint::setMimicJointPosition()
{
    float pos = getPosition() ;
    for( Joint *j: mimic_joints_ ) {
        float mpos = pos * j->mimic_multiplier_ + j->mimic_offset_;
        j->setPosition(mpos) ;
    }
}

float Joint::getPosition() {
    assert(body_) ;
    return static_cast<float>(body_->getJointPos(child_link_->mb_index_)) ;
}

float Joint::getVelocity() {
    assert(body_) ;
    return static_cast<float>(body_->getJointVel(child_link_->mb_index_)) ;
}

float Joint::getTorque() {
    assert(body_) ;
    return static_cast<float>(body_->getJointTorque(child_link_->mb_index_)) ;
}

void Joint::setPosition(float v) {
    assert(body_) ;
    body_->setJointPos(child_link_->mb_index_, static_cast<btScalar>(v)) ;
}

void Joint::setVelocity(float v) {
    assert(body_) ;
    body_->setJointVel(child_link_->mb_index_, static_cast<btScalar>(v)) ;
}

void Joint::setTorque(float v) {
    assert(body_) ;
    body_->addJointTorque(child_link_->mb_index_, static_cast<btScalar>(v)) ;
}

}}
