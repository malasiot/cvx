#include <cvx/viz/renderer/renderer.hpp>
#include <cvx/viz/scene/camera.hpp>
#include <cvx/viz/scene/light.hpp>
#include <cvx/viz/scene/node.hpp>
#include <cvx/viz/scene/marker.hpp>
#include <cvx/viz/scene/material.hpp>
#include <cvx/viz/scene/geometry.hpp>

#include <cvx/viz/gui/glfw_window.hpp>
#include <cvx/viz/gui/trackball.hpp>

#include <cvx/util/math/rng.hpp>
#include <cvx/util/misc/strings.hpp>

#include <GLFW/glfw3.h>
#include <iostream>
#include <thread>
#include "physics.hpp"

using namespace cvx::viz ;
using namespace cvx::util ;

using namespace std ;
using namespace Eigen ;

#define ARRAY_SIZE_Y 5
#define ARRAY_SIZE_X 5
#define ARRAY_SIZE_Z 5

class glfwGUI: public glfwRenderWindow {
public:


    glfwGUI(const ScenePtr &sc, Physics &physics): glfwRenderWindow(), scene_(sc), physics_(physics) {

        Vector3f c{0, 0, 0};
        float r = 10.0 ;

        camera_.reset(new PerspectiveCamera(1.0, 50*M_PI/180, 0.0001, 10*r)) ;
        trackball_.setCamera(camera_, c + Vector3f{0.0, 0, 2*r}, c, {0, 1, 0}) ;
        trackball_.setZoomScale(0.1*r) ;
    }

    void onInit() {

    }




    void onResize(int width, int height) {
        float ratio;
        ratio = width / (float) height;

        trackball_.setScreenSize(width, height);

        static_pointer_cast<PerspectiveCamera>(camera_)->setAspectRatio(ratio) ;

        camera_->setViewport(width, height)  ;
    }


    void onMouseButtonPressed(uint button, size_t x, size_t y, uint flags) override {




        if ( flags & GLFW_MOD_ALT ) {
            Ray ray = camera_->getRay(x, y) ;

            physics_.pickBody(Physics::eigenVectorToBullet(ray.getOrigin()), Physics::eigenVectorToBullet(ray.getDir()*10000));

            Hit hit ;
            if ( scene_->hit(ray, hit) ) {
                cout << hit.node_->name() << endl ;
            }

            picking_ = true ;

        } else {
            switch ( button ) {
                case GLFW_MOUSE_BUTTON_LEFT:
                    trackball_.setLeftClicked(true) ;
                    break ;
                case GLFW_MOUSE_BUTTON_MIDDLE:
                    trackball_.setMiddleClicked(true) ;
                    break ;
                case GLFW_MOUSE_BUTTON_RIGHT:
                    trackball_.setRightClicked(true) ;
                    break ;
            }

            trackball_.setClickPoint(x, y) ;
        }
    }


    void onMouseButtonReleased(uint button, size_t x, size_t y, uint flags) override {



        if ( flags & GLFW_MOD_ALT ) {
            physics_.removePickingConstraint();
            picking_ = false ;
        } else {
            switch ( button ) {
                case GLFW_MOUSE_BUTTON_LEFT:
                    trackball_.setLeftClicked(false) ;
                    break ;
                case GLFW_MOUSE_BUTTON_MIDDLE:
                    trackball_.setMiddleClicked(false) ;
                    break ;
                case GLFW_MOUSE_BUTTON_RIGHT:
                    trackball_.setRightClicked(false) ;
                    break ;
            }


        }

    }

    void onMouseMoved(double xpos, double ypos) override {
        ostringstream s ;
        s << xpos << ',' << ypos ;
        text_ = s.str() ;

        if ( picking_ )  {
            Ray ray = camera_->getRay(xpos, ypos) ;

            physics_.movePickedBody(Physics::eigenVectorToBullet(ray.getOrigin()), Physics::eigenVectorToBullet(ray.getDir()*10000));
        }
        else
            trackball_.setClickPoint(xpos, ypos) ;
    }

    void onMouseWheel(double x) {
        trackball_.setScrollDirection(x>0);
    }


    void onRender(double delta) override {
        trackball_.update() ;

        rdr_.init(camera_) ;
        rdr_.render(scene_) ;

        rdr_.clearZBuffer();

        rdr_.text(text_, 10, 10, Font("arial", 24), {1, 1, 0});

        rdr_.line({0, 0, 0}, {10, 0, 0}, {1, 0, 0, 1});
        rdr_.line({0, 0, 0}, {0, 10, 0}, {0, 1, 0, 1});
        rdr_.line({0, 0, 0}, {0, 0, 10}, {0, 0, 1, 1});

        rdr_.text("X", Vector3f{10, 0, 0}, Font("Arial", 12), Vector3f{1, 0, 0}) ;
        rdr_.text("Y", Vector3f{0, 10, 0}, Font("Arial", 12), Vector3f{0, 1, 0}) ;
        rdr_.text("Z", Vector3f{0, 0, 10}, Font("Arial", 12), Vector3f{0, 0, 1}) ;

        rdr_.circle({0, 0, 0}, {0, 1, 0}, 5.0, {0, 1, 0, 1}) ;

        physics_.stepSimulation(delta);
        physics_.updateTransforms();
        this_thread::yield() ;




    }


    string text_ ;
    Renderer rdr_ ;
    ScenePtr scene_ ;
    TrackBall trackball_ ;
    CameraPtr camera_ ;
    Physics &physics_ ;

    bool picking_ = false ;
};

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

    //m_dynamicsWorld->setGravity(btVector3(0,0,0));
    //m_guiHelper->createPhysicsDebugDrawer(m_dynamicsWorld);

    auto dynamicsWorld = physics.getDynamicsWorld() ;

    if (dynamicsWorld->getDebugDrawer())
        dynamicsWorld->getDebugDrawer()->setDebugMode(btIDebugDraw::DBG_DrawWireframe + btIDebugDraw::DBG_DrawContactPoints);


    Affine3f tr ;
    tr.setIdentity() ;
    tr.translate(Vector3f(0, -50, 0)) ;

    NodePtr node = makeBox("ground", Vector3f{50., 50., 50.}, tr.matrix(), Vector4f{0.5, 0.5, 0.5, 1}) ;

    scene->addChild(node) ;

    btBoxShape* groundShape = physics.createBoxShape(btVector3(btScalar(50.), btScalar(50.), btScalar(50.)));
    physics.m_collisionShapes.push_back(groundShape);

    btTransform groundTransform;
    groundTransform.setIdentity();
    groundTransform.setOrigin(btVector3(0, -50, 0));

    {
        btScalar mass(0.);
        physics.createRigidBody(mass, groundTransform, groundShape, btVector4(0, 0, 1, 1));
    }



    {
        //create a few dynamic rigidbodies
        // Re-using the same collision is better for memory usage and performance

        GeometryPtr geom(new BoxGeometry(.1, .1, .1)) ;




        btBoxShape* colShape = physics.createBoxShape(btVector3(.1, .1, .1));

        //btCollisionShape* colShape = new btSphereShape(btScalar(1.));
        physics.m_collisionShapes.push_back(colShape);

        /// Create Dynamic Objects
        btTransform startTransform;
        startTransform.setIdentity();

        btScalar mass(1.f);

        //rigidbody is dynamic if and only if mass is non zero, otherwise static
        bool isDynamic = (mass != 0.f);

        btVector3 localInertia(0, 0, 0);
        if (isDynamic)
            colShape->calculateLocalInertia(mass, localInertia);

        for (int k = 0; k < ARRAY_SIZE_Y; k++)
        {
            for (int i = 0; i < ARRAY_SIZE_X; i++)
            {
                for (int j = 0; j < ARRAY_SIZE_Z; j++)
                {
                    float tx = 0.2 * i ;
                    float ty = 2 + 0.2 * k ;
                    float tz = 0.2 * j ;

                    startTransform.setOrigin(btVector3(
                        btScalar(tx), btScalar(ty), btScalar(tz)));

                    btRigidBody *body = physics.createRigidBody(mass, startTransform, colShape);

                    NodePtr node(new Node) ;

                    std::shared_ptr<PhongMaterialParameters> params(new PhongMaterialParameters) ;
                    params->setDiffuse({g_rng.uniform(0.0, 1.), 1, g_rng.uniform(0., 1.), 1}) ;

                    MaterialInstancePtr material(new PhongMaterialInstance(params)) ;

                    DrawablePtr dr(new Drawable(geom, material)) ;

                    node->addDrawable(dr) ;

                    node->setPickable(true);


                    node->setName(cvx::util::format("%d %d %d", i, j, k)) ;

                    Affine3f tr ;
                    tr.setIdentity() ;
                    tr.translate(Vector3f{tx, ty, tz}) ;
                    node->matrix() = tr.matrix() ;

                    scene->addChild(node) ;

                    physics.addTransformObserver(body, [node](const Affine3f &tr) { node->matrix() = tr ;} ) ;
                }
            }
        }
    }


}

int main(int argc, char *argv[]) {


    createScene() ;

    glfwGUI gui(scene, physics) ;

    gui.run(640, 480) ;

}
