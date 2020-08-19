#ifndef GLFW_VIEWER_HPP
#define GLFW_VIEWER_HPP__

#include <string>
#include <cvx/viz/gui/glfw_window.hpp>
#include <cvx/viz/scene/scene_fwd.hpp>
#include <cvx/viz/renderer/renderer.hpp>
#include <cvx/viz/gui/trackball.hpp>

class GLFWwindow ;

namespace cvx { namespace viz {

class GLFWViewer: public glfwRenderWindow {
public:
    GLFWViewer(ScenePtr scene);

    void onInit() override;

    void onResize(int width, int height) override;

    void onMouseButtonPressed(uint button, size_t x, size_t y, uint flags) override;

    void onMouseButtonReleased(uint button, size_t x, size_t y, uint flags) override;

    void onMouseMoved(double xpos, double ypos) override;

    void onMouseWheel(double x) override;

    void onUpdate(double delta) override;

    std::string text_ ;
    Renderer rdr_ ;
    ScenePtr scene_ ;
    TrackBall trackball_ ;
    CameraPtr camera_ ;
} ;

} // namespace viz
} // namespace cvx

#endif
