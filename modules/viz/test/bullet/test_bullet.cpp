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
#include "physics.hpp"
#include "bullet_gui.hpp"

#include <QApplication>
#include <QMainWindow>

using namespace cvx::viz ;
using namespace cvx::util ;

using namespace std ;
using namespace Eigen ;

#define ARRAY_SIZE_Y 5
#define ARRAY_SIZE_X 5
#define ARRAY_SIZE_Z 5

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

Physics physics ;
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

    physics.createEmptyDynamicsWorld();

    Affine3f tr ;
    tr.setIdentity() ;
    tr.translate(Vector3f(0, -50, 0)) ;

    NodePtr node = makeBox("ground", Vector3f{50., 50., 50.}, tr.matrix(), Vector4f{0.5, 0.5, 0.5, 1}) ;

    scene->addChild(node) ;

    btCollisionShape *groundShape = physics.createBoxShape(btVector3(btScalar(50.), btScalar(50.), btScalar(50.)));

    btTransform groundTransform;
    groundTransform.setIdentity();
    groundTransform.setOrigin(btVector3(0, -50, 0));

    physics.createStaticRigidBody(groundTransform, groundShape);


    //create a few dynamic rigidbodies
    // Re-using the same collision is better for memory usage and performance

    GeometryPtr geom(new BoxGeometry(.1, .1, .1)) ;

    btCollisionShape *colShape = physics.createBoxShape(btVector3(.1, .1, .1));

    /// Create Dynamic Objects
    ///
    btTransform startTransform;
    startTransform.setIdentity();
    btScalar mass(1.f);
    btVector3 localInertia(0, 0, 0);

    colShape->calculateLocalInertia(mass, localInertia);

    for (int k = 0; k < ARRAY_SIZE_Y; k++)  {
        for (int i = 0; i < ARRAY_SIZE_X; i++)  {
            for (int j = 0; j < ARRAY_SIZE_Z; j++) {
                float tx = 0.2 * i ;
                float ty = 2 + 0.2 * k ;
                float tz = 0.2 * j ;

                startTransform.setOrigin(btVector3(btScalar(tx), btScalar(ty), btScalar(tz)));

                NodePtr node(new Node) ;

                PhongMaterialInstance *pm = new PhongMaterialInstance();
                pm->setDiffuse({g_rng.uniform(0.0, 1.), 1, g_rng.uniform(0., 1.), 1}) ;

                MaterialInstancePtr material(pm) ;

                DrawablePtr dr(new Drawable(geom, material)) ;

                node->addDrawable(dr) ;

                node->setPickable(true);

                node->setName(cvx::util::format("%d %d %d", i, j, k)) ;

                Affine3f tr ;
                tr.setIdentity() ;
                tr.translate(Vector3f{tx, ty, tz}) ;
                node->matrix() = tr.matrix() ;

               scene->addChild(node) ;

                physics.createRigidBody(mass, node, colShape, localInertia);
            }
        }
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
    window.resize(512, 512) ;
    window.show() ;

    return app.exec();
}
