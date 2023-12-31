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

#include "bullet_gui.hpp"

using namespace Eigen ;

using namespace std ;
using namespace cvx::viz ;
using namespace cvx::util ;


class GUI: public SimulationGui {
public:
    GUI(cvx::viz::ScenePtr scene, cvx::viz::PhysicsWorld &physics, vector<RigidBodyPtr> &objects):
    SimulationGui(scene, physics), objects_(std::move(objects)) {

    }

    void onUpdate(float delta) override {
        SimulationGui::onUpdate(delta) ;

        vector<ContactResult> contacts ;
        if ( physics_.contactTest(objects_[0], contacts ) ) {
            for( ContactResult &c: contacts ) {

                    cout << c.a_->getName() << ' ' << c.b_->getName() << endl ;
            }
        }

    }

private:
    vector<RigidBodyPtr> objects_ ;
};

static PhysicsWorld physics ;
static ScenePtr scene ;

static const float chain_radius = 0.1 ;
static const float chain_length = 0.3 ;
static const int TOTAL_BOXES = 7;


static Vector4f sColors[4] =  {
    Vector4f(60. / 256., 186. / 256., 84. / 256., 1),
    Vector4f(244. / 256., 194. / 256., 13. / 256., 1),
    Vector4f(219. / 256., 50. / 256., 54. / 256., 1),
    Vector4f(72. / 256., 133. / 256., 237. / 256., 1),

};

vector<RigidBodyPtr> boxes;

void createScene() {
    scene.reset(new Scene) ;

    // init physics

    physics.createDefaultDynamicsWorld();

    // add light
    std::shared_ptr<DirectionalLight> dl( new DirectionalLight(Vector3f(0.5, 0.5, 1)) ) ;
    dl->diffuse_color_ = Vector3f(1, 1, 1) ;
    scene->addLight(dl) ;

    // create ground plane object

    Affine3f tr(Translation3f{0, -50, 0}) ;

    Vector3f ground_hs{50., 50., 50.} ;
    scene->addBox(ground_hs, tr.matrix(), {0.5, 0.5, 0.5, 1})->setName("ground") ;
    RigidBodyPtr ground = make_shared<RigidBody>(CollisionShapePtr(new BoxCollisionShape(ground_hs)), tr);
    ground->setName("ground") ;
    physics.addRigidBody(ground) ;

    // create static pole
 //   Affine3f poleTransform(Translation3f{0.5, 5, 0}) ;
 //   scene->addCylinder(0.25, 10, poleTransform.matrix(), {0, 1, 0, 1})->setName("pole") ;
 //   RigidBody pole(CylinderCollisionShape(0.25, 10), poleTransform);
 //   pole.setName("pole") ;
 //   physics.addBody(pole) ;

    // create static box from mesh
    Affine3f meshTransform(Translation3f{2.5, 0.5, 1.5}) ;

    NodePtr meshNode(new Node) ;
    meshNode->load("/home/malasiot/Downloads/CoffeeTable.obj", 0, 3.5f) ;
    meshNode->matrix() = meshTransform;
    scene->addChild(meshNode) ;

    CollisionShapePtr meshColShape(new StaticMeshCollisionShape("/home/malasiot/Downloads/CoffeeTable.obj")) ;
    meshColShape->setLocalScale(3.5);

    RigidBodyPtr mesh = make_shared<RigidBody>(1.0, new UpdateSceneMotionState(meshNode), meshColShape);
    mesh->setName("mesh") ;
    physics.addRigidBody(mesh) ;

    // create collision shape for chain element

    btScalar mass(1.0) ;

    CollisionShapePtr colShape(new CylinderCollisionShape(chain_radius, chain_length)) ;

    int lastBoxIndex = TOTAL_BOXES - 1;

    for (int i = 0; i < TOTAL_BOXES; i++) {
        float tx = 0, ty = 2.0f+ i*1.5*chain_length, tz = 0 ;

        Affine3f box_tr(Translation3f{tx, ty, tz}) ;

        NodePtr chain_node = scene->addCylinder(chain_radius, chain_length, box_tr.matrix(), sColors[i%4]);
        string name = cvx::util::format("chain {}", i) ;
        chain_node->setName(name) ;

        if ( i== lastBoxIndex ) {
            RigidBodyPtr box(new RigidBody(colShape, box_tr)) ;
            box->setName(name) ;
            physics.addRigidBody(box) ;
            boxes.push_back(box) ;
        }
        else {
            RigidBodyPtr box(new RigidBody(mass, new UpdateSceneMotionState(chain_node), colShape)) ;
            box->setName(name) ;
            physics.addRigidBody(box) ;
            boxes.push_back(box) ;
        }

    }
    //add N-1 spring constraints
    for (int i = 0; i < TOTAL_BOXES - 1; ++i) {
        Point2PointConstraint c(boxes[i], boxes[i+1], {0.0, 1.5*chain_length/2.0, 0}, {0.0, -1.5*chain_length/2.0, 0}) ;
        physics.addConstraint(c);
    }

}

int main(int argc, char **argv)
{
    createScene() ;

    QApplication app(argc, argv);

    SimpleQtViewer::initDefaultGLContext();

    QMainWindow window ;
    window.setCentralWidget(new GUI(scene, physics, boxes)) ;
    window.resize(1024, 1024) ;
    window.show() ;

    return app.exec();
}
