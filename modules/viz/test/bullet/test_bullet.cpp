#include <cvx/viz/renderer/renderer.hpp>
#include <cvx/viz/scene/camera.hpp>
#include <cvx/viz/scene/light.hpp>
#include <cvx/viz/scene/node.hpp>
#include <cvx/viz/scene/marker.hpp>
#include <cvx/viz/scene/material.hpp>
#include <cvx/viz/scene/geometry.hpp>

#include <cvx/util/math/rng.hpp>
#include <cvx/util/misc/format.hpp>

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

#define ARRAY_SIZE_Y 2
#define ARRAY_SIZE_X 2
#define ARRAY_SIZE_Z 2

PhysicsWorld physics ;
ScenePtr scene(new Scene) ;

RNG g_rng ;

void createScene() {

    // create new scene and add light

    scene->addMarkerInstance(MarkerInstancePtr(new SphereMarkerInstance({0, -1, 0}, 0.2, {1, 0, 0})));

    std::shared_ptr<DirectionalLight> dl( new DirectionalLight(Vector3f(0.5, 0.5, 1)) ) ;
    dl->diffuse_color_ = Vector3f(1, 1, 1) ;
    scene->addLight(dl) ;

    scene->setPickable(true);

    // init physics

    physics.createDefaultDynamicsWorld();

    Affine3f tr(Translation3f{0, -50, 0}) ;

    Vector3f ground_hs{50.0f, 50.0f, 50.0f} ;
    scene->addBox(ground_hs, tr.matrix(), Vector4f{0.5, 0.5, 0.5, 1}) ;
    physics.addBody(make_shared<RigidBody>(CollisionShapePtr(new BoxCollisionShape(ground_hs)), tr)) ;

    //create a few dynamic rigidbodies
    // Re-using the same collision is better for memory usage and performance

    Vector3f cube_hs{.1f, .1f, .1f} ;
    GeometryPtr geom(new BoxGeometry(cube_hs)) ;
    CollisionShapePtr colShape(new BoxCollisionShape(cube_hs));
    btScalar mass(1.f);

    /// Create Dynamic Objects
    ///
    for (int k = 0; k < ARRAY_SIZE_Y; k++)  {
        for (int i = 0; i < ARRAY_SIZE_X; i++)  {
            for (int j = 0; j < ARRAY_SIZE_Z; j++) {
                float tx = 0.2 * i ;
                float ty = 2 + 0.2 * k ;
                float tz = 0.2 * j ;

                Affine3f tr(Translation3f{tx, ty, tz}) ;

                NodePtr node = scene->addBox(cube_hs, tr.matrix(), {g_rng.uniform(0.0, 1.), g_rng.uniform(0., 1.),  g_rng.uniform(0., 1.), 1}) ;

                node->setPickable(true);
                node->setName(cvx::util::format("%d %d %d", i, j, k)) ;

                physics.addBody(make_shared<RigidBody>(mass, new UpdateSceneMotionState(node), colShape));
            }
        }
    }



}

int main(int argc, char **argv)
{
    createScene() ;

    QApplication app(argc, argv);

    cvx::viz::SimpleQtViewer::initDefaultGLContext() ;

    QMainWindow window ;
    window.setCentralWidget(new SimulationGui(scene, physics)) ;
    window.resize(512, 512) ;
    window.show() ;

    return app.exec();
}
