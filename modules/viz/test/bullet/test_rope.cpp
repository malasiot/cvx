/* these should be included before QtGui */
/* there will be a warning that you cannot mix glew with Qt GL which you can ignore */

#include <cvx/viz/scene/material.hpp>

#include <QApplication>
#include <QMainWindow>

#include <cvx/util/misc/format.hpp>
#include <cvx/util/math/rng.hpp>

#include <cvx/viz/scene/camera.hpp>
#include <cvx/viz/scene/light.hpp>

#include <cvx/viz/scene/node.hpp>
#include <cvx/viz/scene/geometry.hpp>

#include <cvx/viz/physics/world.hpp>

#include <iostream>

#include "physics.hpp"
#include "bullet_gui.hpp"

using namespace Eigen ;

using namespace std ;
using namespace cvx::viz ;
using namespace cvx::util ;


static PhysicsWorld physics ;
static ScenePtr scene ;



NodePtr makeBox(const string &name, const Vector3f &hs, const Matrix4f &tr, const Vector4f &clr) {

    NodePtr box_node(new Node) ;
    box_node->setName(name) ;

    GeometryPtr geom(new BoxGeometry(hs)) ;

    MaterialInstancePtr material(new ConstantMaterialInstance(clr)) ;

    DrawablePtr dr(new Drawable(geom, material)) ;

    box_node->addDrawable(dr) ;

    box_node->matrix() = tr ;

    return box_node ;
}

NodePtr makeCylinder(const string &name, float radius, float length, const Matrix4f &tr, const Vector4f &clr) {

    // we need an extra node to perform rotation of cylinder so that it is aligned with Y axis instead of Z

    NodePtr node(new Node) ;
    node->setName(name) ;

    GeometryPtr geom(new CylinderGeometry(radius, length)) ;

    MaterialInstancePtr material(new ConstantMaterialInstance(clr)) ;

    DrawablePtr dr(new Drawable(geom, material)) ;

    node->addDrawable(dr) ;

    node->matrix().rotate(AngleAxisf(-0.5*M_PI, Vector3f::UnitX()));

    NodePtr externalNode(new Node) ;
    externalNode->matrix() = tr ;
    externalNode->addChild(node) ;

    return externalNode ;
}

static const float chain_radius = 0.1 ;
static const float chain_length = 0.3 ;
static const int TOTAL_BOXES = 8;


static Vector4f sColors[4] =  {
    Vector4f(60. / 256., 186. / 256., 84. / 256., 1),
    Vector4f(244. / 256., 194. / 256., 13. / 256., 1),
    Vector4f(219. / 256., 50. / 256., 54. / 256., 1),
    Vector4f(72. / 256., 133. / 256., 237. / 256., 1),

};

void createScene() {
    scene.reset(new Scene) ;

    // init physics

    physics.createDefaultDynamicsWorld();

    // add light
    std::shared_ptr<DirectionalLight> dl( new DirectionalLight(Vector3f(0.5, 0.5, 1)) ) ;
    dl->diffuse_color_ = Vector3f(1, 1, 1) ;
    scene->addLight(dl) ;

    // create ground plane object

    Affine3f tr ;
    tr.setIdentity() ;
    tr.translate(Vector3f(0, -50, 0)) ;

    NodePtr node = makeBox("ground", Vector3f{50., 50., 50.}, tr.matrix(), Vector4f{0.5, 0.5, 0.5, 1}) ;

    scene->addChild(node) ;

    /// create ground collision shape
    CollisionShape groundShape = physics.createBoxShape(btVector3(btScalar(50.), btScalar(50.), btScalar(50.)));

    btTransform groundTransform;
    groundTransform.setIdentity();
    groundTransform.setOrigin(btVector3(0, -50, 0));

    RigidBody ground(groundShape, tr) ;
    physics.addBody(ground) ;

    CollisionShape poleShape = physics.createCylinderShape(0.25, 10);

    Affine3f poleTransform(Translation3f(Vector3f{0.5, 5, 0})) ;

    RigidBody pole(poleShape, poleTransform);
    physics.addBody(pole) ;

    NodePtr pole_node = makeCylinder("pole", 0.25, 10, poleTransform.matrix(), {0, 1, 0, 1}) ;
    scene->addChild(pole_node) ;

    // create collision shape for chain element
    CollisionShape colShape = physics.createCylinderShape(chain_radius, chain_length) ;

    btScalar mass(1.0) ;
    vector<RigidBody> boxes;

    int lastBoxIndex = TOTAL_BOXES - 1;
    for (int i = 0; i < TOTAL_BOXES; ++i) {
        float tx = 0, ty = 2+ i*chain_length, tz = 0 ;

        Affine3f box_tr ;
        box_tr.setIdentity() ;
        box_tr.translate(Vector3f{tx, ty, tz}) ;

        NodePtr box_node = makeCylinder(cvx::util::format("box-%d", i), chain_radius, chain_length, box_tr.matrix(), sColors[i%3]) ;
        scene->addChild(box_node) ;

        if ( i== lastBoxIndex ) {
            RigidBody box(colShape, box_tr) ;
            physics.addBody(box) ;
            boxes.push_back(box) ;
        }
        else {
            RigidBody box(mass, new UpdateSceneMotionState(box_node), colShape) ;
            physics.addBody(box) ;
            boxes.push_back(box) ;
        }

    }

    btDynamicsWorld *world = physics.getDynamicsWorld() ;

    //add N-1 spring constraints
    for (int i = 0; i < TOTAL_BOXES - 1; ++i) {
        btRigidBody* b1 = boxes[i].handle();
        btRigidBody* b2 = boxes[i + 1].handle();

        btPoint2PointConstraint* leftSpring = new btPoint2PointConstraint(*b1, *b2, btVector3(0.0, chain_length/2.0, 0), btVector3(0.0, -chain_length/2.0, 0));

        world->addConstraint(leftSpring);


    }

}

int main(int argc, char **argv)
{

    createScene() ;

    QApplication app(argc, argv);

    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setMajorVersion(3);
    format.setMinorVersion(3);
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    format.setSwapInterval(1);

    format.setSamples(4);
    format.setProfile(QSurfaceFormat::CoreProfile);

    QSurfaceFormat::setDefaultFormat(format);

    QMainWindow window ;
    window.setCentralWidget(new TestAnimation(scene, physics)) ;
    window.show() ;

    return app.exec();
}
