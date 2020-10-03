#include <cvx/viz/gui/glfw_viewer.hpp>

#include <cvx/viz/renderer/renderer.hpp>
#include <cvx/viz/scene/camera.hpp>
#include <cvx/viz/scene/light.hpp>
#include <cvx/viz/scene/node.hpp>
#include <cvx/viz/scene/marker.hpp>
#include <cvx/viz/renderer/gl/shader.hpp>
#include <cvx/viz/scene/material.hpp>
#include <cvx/viz/scene/geometry.hpp>
#include <cvx/viz/scene/mesh.hpp>

#include <cvx/viz/gui/glfw_window.hpp>
#include <cvx/viz/gui/trackball.hpp>

#include <GLFW/glfw3.h>

using namespace std ;
using namespace Eigen ;

namespace cvx { namespace viz {
cvx::viz::GLFWViewer::GLFWViewer(cvx::viz::ScenePtr scene): glfwRenderWindow(), scene_(scene) {
    auto c = scene->geomCenter() ;
    auto r = scene->geomRadius(c) ;

    camera_.reset(new PerspectiveCamera(1.0, 50*M_PI/180, 0.0001, 10*r)) ;
    trackball_.setCamera(camera_, c + Vector3f{0.0, 0, 2*r}, c, {0, 1, 0}) ;
    trackball_.setZoomScale(0.1*r) ;
    rdr_.setCamera(camera_) ;
}

void GLFWViewer::onInit() {

}

void GLFWViewer::onResize(int width, int height) {
    float ratio;
    ratio = width / (float) height;

    trackball_.setScreenSize(width, height);

    static_pointer_cast<PerspectiveCamera>(camera_)->setAspectRatio(ratio) ;

    camera_->setViewport(width, height)  ;
}

void GLFWViewer::onMouseButtonPressed(uint button, size_t x, size_t y, uint flags) {
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
    trackball_.update() ;

}

void GLFWViewer::onMouseButtonReleased(uint button, size_t x, size_t y, uint flags) {
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
    trackball_.update() ;

}

void GLFWViewer::onMouseMoved(double xpos, double ypos) {
    trackball_.setClickPoint(xpos, ypos) ;
    trackball_.update() ;
}

void GLFWViewer::onMouseWheel(double x) {
    trackball_.setScrollDirection(x>0);
    trackball_.update() ;
}

void GLFWViewer::onUpdate(double delta) {
     rdr_.render(scene_) ;
}
} // namespace viz
} // namespace cvx
