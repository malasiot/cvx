#include "renderer_impl.hpp"
#include "font_manager.hpp"
#include "glyph_cache.hpp"
#include "util.hpp"

#include <iostream>
#include <cstring>

#include <Eigen/Dense>

#include <fstream>
#include <memory>
#include <set>

#include <cvx/viz/scene/node.hpp>
#include <cvx/viz/scene/drawable.hpp>
#include <cvx/viz/scene/mesh.hpp>
#include <cvx/viz/scene/camera.hpp>
#include <cvx/viz/scene/light.hpp>
#include <cvx/viz/scene/material.hpp>
#include <cvx/viz/scene/geometry.hpp>

#include <cvx/util/misc/strings.hpp>

using namespace std ;
using namespace Eigen ;
using namespace cvx::util ;

namespace cvx { namespace viz { namespace detail {


#define POSITION_LOCATION    0
#define NORMALS_LOCATION    1
#define COLORS_LOCATION    2
#define BONE_ID_LOCATION    3
#define BONE_WEIGHT_LOCATION    4
#define UV_LOCATION 5

void RendererImpl::setCamera(const CameraPtr &cam) {
    cam_ = cam ;
}

static const char *shadow_map_shader_vs = R"(
  layout (location = 0) in vec3 aPos;

  uniform mat4 lightSpaceMatrix;
  uniform mat4 model;

  void main()
  {
     gl_Position = lightSpaceMatrix * model * vec4(aPos, 1.0);
  }
)" ;

static const char *shadow_map_shader_fs = R"(
  void main()
  {
  }
)" ;

static const char *shadow_debug_shader_vs = R"(
  layout (location = 0) in vec3 aPos;
  layout (location = 1) in vec2 aTexCoords;

  out vec2 TexCoords;

  void main()
  {
     TexCoords = aTexCoords;
     gl_Position = vec4(aPos, 1.0);
  }
)" ;

static const char *shadow_debug_shader_fs = R"(
  out vec4 FragColor;
  in vec2 TexCoords;

  uniform sampler2D depthMap;
  uniform float near_plane;
  uniform float far_plane;

  // required when using a perspective projection matrix
  float LinearizeDepth(float depth)
  {
     float z = depth * 2.0 - 1.0; // Back to NDC
     return (2.0 * near_plane * far_plane) / (far_plane + near_plane - z * (far_plane - near_plane));
  }

  void main()
  {
     float depthValue = texture(depthMap, TexCoords).r;
     // FragColor = vec4(vec3(LinearizeDepth(depthValue) / far_plane), 1.0); // perspective
     FragColor = vec4(vec3(depthValue), 1.0); // orthographic
  }
)" ;

const GLuint shadow_map_width = 1024 ;
const GLuint shadow_map_height = 1024 ;

void RendererImpl::setupShadows(const Vector3f &ldir) {
    Matrix4f lightProjection = ortho(-10.0f, 10.0f, -10.0f, 10.0f, -10.0f, 10.0f);
    Matrix4f lightView = lookAt(ldir, Vector3f{0, 0, 0}, Vector3f(0.0, 1.0, 0.0));
    ls_mat_ = lightProjection * lightView;

    if ( !shadow_map_.ready() ) {
        shadow_map_.init(shadow_map_width, shadow_map_height) ;
        shadow_map_shader_.reset(new OpenGLShaderProgram(shadow_map_shader_vs, shadow_map_shader_fs));
        shadow_debug_shader_.reset(new OpenGLShaderProgram(shadow_debug_shader_vs, shadow_debug_shader_fs));
    }
}


unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
            // positions        // texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}


void RendererImpl::renderScene(const ScenePtr &scene) {

    // render node hierarchy

    glEnable(GL_DEPTH_TEST) ;
    glDepthFunc(GL_LESS);

    glEnable(GL_CULL_FACE) ;
    glCullFace(GL_BACK) ;
    glFrontFace(GL_CCW) ;

    glEnable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Vector4f bg_clr = cam_->bgColor() ;

    glClearColor(bg_clr.x(), bg_clr.y(), bg_clr.z(), bg_clr.w()) ;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    if ( const auto &pcam = dynamic_pointer_cast<PerspectiveCamera>(cam_) ) {
        perspective_ = pcam->projectionMatrix() ;
        znear_ = pcam->zNear() ;
        zfar_ = pcam->zFar() ;
    }

    const Viewport &vp = cam_->getViewport() ;
    glViewport(vp.x_, vp.y_, vp.width_, vp.height_);

    proj_ = cam_->getViewMatrix() ;

    if ( has_shadows_ && has_shadow_light_ )
        shadow_map_.bindTexture(GL_TEXTURE1);

    render(scene, scene, Matrix4f::Identity()) ;
}

void RendererImpl::renderShadowMap(const ScenePtr &scene) {

    Vector3f ldir ;
    has_shadow_light_ = false ;

    auto lights = scene->lights() ;
    if ( !lights.empty() ) {
        if ( DirectionalLight *dl = dynamic_cast<DirectionalLight *>(lights[0].get()) ) {
            ldir = dl->direction_ ;
            has_shadow_light_ = true ;
        } else if ( SpotLight *sl = dynamic_cast<SpotLight *>(lights[0].get()) ) {
            ldir = sl->direction_ ;
            has_shadow_light_ = true ;
        } else if ( PointLight *pl = dynamic_cast<PointLight *>(lights[0].get()) ) {
            ldir = pl->position_ ;
            has_shadow_light_ = true ;
        }
    }


    setupShadows(ldir);
    // render node hierarchy

    shadow_map_.bind();
     glViewport(0, 0, shadow_map_width, shadow_map_height);

    shadow_map_.bindTexture(GL_TEXTURE0) ;
    glClear(GL_DEPTH_BUFFER_BIT);
    glCullFace(GL_FRONT);

    renderShadowMap(scene, scene, Matrix4f::Identity()) ;

    shadow_map_.unbind(default_fbo_) ;
}

void RendererImpl::render(const ScenePtr &scene, const NodePtr &node, const Matrix4f &tf) {

    if ( !node->isVisible() ) return ;

    Matrix4f mat = node->matrix().matrix(),
            tr = tf * mat ; // accumulate transform

    for( uint i=0 ; i<node->numDrawables() ; i++ ) {
        const DrawablePtr &m = node->getDrawable(i) ;
        render(scene, m, tr) ;
    }

    for( uint i=0 ; i<node->numChildren() ; i++ ) {
        const NodePtr &n = node->getChild(i) ;
        render(scene, n, tr) ;
    }
}

void RendererImpl::renderShadowMap(const ScenePtr &scene, const NodePtr &node, const Matrix4f &tf) {

    if ( !node->isVisible() ) return ;

    Matrix4f mat = node->matrix().matrix(),
            tr = tf * mat ; // accumulate transform

    for( uint i=0 ; i<node->numDrawables() ; i++ ) {
        const DrawablePtr &m = node->getDrawable(i) ;
        renderShadowMap(scene,  m, tr) ;
    }

    for( uint i=0 ; i<node->numChildren() ; i++ ) {
        const NodePtr &n = node->getChild(i) ;
        renderShadowMap(scene,  n, tr) ;
    }
}


RendererImpl::RendererImpl(int flags):  flags_(flags), default_material_(new PhongMaterialInstance) {

}

RendererImpl::~RendererImpl() {

}


#define MAX_LIGHTS 10

void RendererImpl::setLights(const NodePtr &node, const Affine3f &parent_tf, const MaterialInstancePtr &mat)
{
    Affine3f tf = parent_tf * node->matrix() ;

    for( uint i=0 ; i< node->lights().size() ; i++  ) {
        if ( light_index_ >= MAX_LIGHTS ) return ;
        const LightPtr &light = node->lights()[i] ;

        mat->applyLight(light_index_, light, tf) ;
        light_index_ ++ ;
    }

    for( uint i=0 ; i<node->numChildren() ; i++ )
        setLights(node->getChild(i), tf, mat) ;
}

void RendererImpl::setLights(const ScenePtr &scene, const MaterialInstancePtr &material) {
    light_index_ = 0 ;

    Isometry3f mat ;
    mat.setIdentity() ;

    setLights(scene, mat, material) ;
}

void RendererImpl::setPose(const MeshPtr &mesh, const MaterialInstancePtr &material) {
    const auto &skeleton = mesh->skeleton() ;
    for( int i=0 ; i<skeleton.size() ; i++ ) {
        const Bone &b = skeleton[i] ;
        material->applyBoneTransform(i, ( b.node_->globalTransform() * b.offset_).matrix()) ;
    }
}

void RendererImpl::drawMeshData(MeshData &data, GeometryPtr geom) {

    glBindVertexArray(data.vao_);

    MeshPtr mesh = geom->getMesh() ;

    if ( mesh ) data.update(*mesh) ;

    if ( mesh ) {
        if ( mesh->ptype() == Mesh::Triangles ) {
            if ( mesh->vertices().hasIndices() ) {
                // bind index buffer if you want to render indexed data
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data.index_);
                // indexed draw call
                glDrawElements(GL_TRIANGLES, data.indices_, GL_UNSIGNED_INT, nullptr);
            }
            else
                glDrawArrays(GL_TRIANGLES, 0, data.elem_count_) ;
        }
        else if ( mesh->ptype() == Mesh::Lines ) {
            glDrawArrays(GL_LINES, 0, data.elem_count_) ;
        }
        else if ( mesh->ptype() == Mesh::Points ) {
            glDrawArrays(GL_POINTS, 0, data.elem_count_) ;
        }

    } else {
        glDrawArrays(GL_TRIANGLES, 0, data.elem_count_) ;
    }

    glBindVertexArray(0) ;

    glFlush();
}


void RendererImpl::render(const ScenePtr &scene, const DrawablePtr &geom, const Matrix4f &mat)
{
    if ( !geom->geometry() ) return ;

    MeshPtr mesh = geom->geometry()->getMesh() ;

    MeshData *data = mesh->getMeshData() ;

    if ( !data ) return ;

    MaterialInstancePtr material = geom->material() ;
    if ( !material ) material = default_material_ ;

    if ( has_shadows_ )
        material->setFlags(HAS_SHADOWS) ;

    material->use() ;
    material->applyParameters() ;
    material->applyTransform(perspective_, proj_, mat) ;
    material->applyShadow(ls_mat_, shadow_bias_) ;
    setLights(scene, material) ;

    if ( mesh && mesh->hasSkeleton() )
        setPose(mesh, material) ;

#if 0

    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, data.tf_);

    glEnable(GL_RASTERIZER_DISCARD);

    glBeginTransformFeedback(GL_TRIANGLES);

    drawMeshData(*data, geom->geometry()) ;

    glEndTransformFeedback();

    glDisable(GL_RASTERIZER_DISCARD);

    vector<GLfloat> fdata(36, 0) ;

    glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, 0, 36*sizeof(GLfloat), fdata.data());

    glBindVertexArray(0) ;
#else
    drawMeshData(*data, geom->geometry()) ;

#endif
    // glUseProgram(0) ;
}

void RendererImpl::renderShadowMap(const ScenePtr &scene, const DrawablePtr &geom, const Matrix4f &mat)
{
    if ( !geom->geometry() ) return ;

    MeshPtr mesh = geom->geometry()->getMesh() ;

    MeshData *data = mesh->getMeshData() ;

    if ( !data ) return ;

    shadow_map_shader_->use() ;

    shadow_map_shader_->setUniform("lightSpaceMatrix", ls_mat_);
    shadow_map_shader_->setUniform("model", mat) ;
    drawMeshData(*data, geom->geometry()) ;

    // glUseProgram(0) ;
}

void RendererImpl::clearZBuffer()
{
    glClear(GL_DEPTH_BUFFER_BIT) ;
}


cv::Mat RendererImpl::getColor(bool alpha)
{
    GLint v[4] ;
    glGetIntegerv(GL_VIEWPORT, v) ;

    uint width = v[2], height = v[3] ;

    if ( alpha )
    {
        cv::Mat_<cv::Vec4b> mask(height, width) ;
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, mask.ptr());

        cv::flip(mask, mask, 0) ;

        cv::cvtColor(mask, mask, cv::COLOR_RGBA2BGRA);

        return mask ;
    }
    else
    {
        cv::Mat_<cv::Vec3b> mask(height, width) ;
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, mask.ptr());

        cv::flip(mask, mask, 0) ;

        cv::cvtColor(mask, mask, cv::COLOR_RGB2BGR);

        return mask ;
    }
}

cv::Mat RendererImpl::getColor(cv::Mat &bg, float alpha)
{
    GLint v[4] ;
    glGetIntegerv(GL_VIEWPORT, v) ;

    uint width = v[2], height = v[3] ;

    cv::Mat_<cv::Vec3b> mask(height, width) ;
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, mask.ptr());
    cv::flip(mask, mask, 0) ;
    cv::cvtColor(mask, mask, cv::COLOR_RGB2BGR);

    cv::Mat dst ;
    cv::addWeighted( mask, alpha, bg, (1 - alpha), 0.0, dst);

    return dst ;
}

cv::Mat RendererImpl::getDepth() {

    GLint v[4] ;
    glGetIntegerv(GL_VIEWPORT, v) ;

    uint width = v[2], height = v[3] ;

    cv::Mat_<float> depth(height, width);

    glReadBuffer(GL_DEPTH_ATTACHMENT);

    glReadPixels(0, 0, width, height, GL_DEPTH_COMPONENT, GL_FLOAT, depth.ptr());

    cv::Mat_<float>::iterator it = depth.begin(), end = depth.end();
    float max_allowed_z = zfar_ * 0.99;


    for (unsigned int j = 0; j < height; ++j)
        for (unsigned int i = 0; i < width; ++i, ++it)
        {
            //need to undo the depth buffer mapping
            //http://olivers.posterous.com/linear-depth-in-glsl-for-real
            float z  = 2 * zfar_ * znear_ / (zfar_ + znear_ - (zfar_ - znear_) * (2 * (*it) - 1));

            if (z > max_allowed_z) *it = 0;
            else {
                *it = z ;
            }
        }
    // Rescale the depth to be in millimeters
    cv::Mat depth_scale(cv::Size(width, height), CV_16UC1);
    depth.convertTo(depth_scale, CV_16UC1, 1e3);

    cv::flip(depth_scale, depth_scale, 0) ;

    // cv::imwrite("/tmp/dmap.png", vl::depthViz(depth_scale)) ;
    return depth_scale;

}


void RendererImpl::renderText(const string &text, float x, float y, const Font &font, const Vector3f &clr) {
    using namespace detail ;

    if ( text.empty() ) return ;

    TextItem ti(text, font) ;
    ti.render(x, y, clr) ;
}

void RendererImpl::renderText(const string &text, const Vector3f &pos, const Font &font, const Vector3f &clr) {
    using namespace detail ;

    if ( text.empty() ) return ;

    TextItem ti(text, font) ;

    Vector3f p = Affine3f(perspective_ * proj_) * pos ;

    GLint vp[4] ;
    glGetIntegerv (GL_VIEWPORT, vp) ;

    float xn = p.x()/p.z(), yn = p.y()/p.z() ;
    float x = (xn + 1.0) * (vp[2]/2.0) + vp[0] ;
    float y = (yn + 1.0) * (vp[3]/2.0) + vp[1];

    ti.render(x, y, clr) ;
}

void RendererImpl::renderTextObject(const Text &text, float x, float y, const Font &face, const Vector3f &clr)
{
    text.impl_->render(x, y, clr) ;
}

void RendererImpl::renderTextObject(const Text &text, const Vector3f &pos , const Font &face, const Vector3f &clr)
{
    Vector3f p = Affine3f(perspective_ * proj_) * pos ;

    GLint vp[4] ;
    glGetIntegerv (GL_VIEWPORT, vp) ;

    float xn = p.x()/p.z(), yn = p.y()/p.z() ;
    float x = (xn + 1.0) * (vp[2]/2.0) + vp[0] ;
    float y = (yn + 1.0) * (vp[3]/2.0) + vp[1];

    text.impl_->render(x, y, clr) ;
}

static const char * line_shader_vertex_ = R"(

uniform mat4 mvp;
uniform vec4 colour;
in vec4 position;
out vec4 colourV;

void main (void) {
  colourV = colour;
  gl_Position = mvp * position;
}
)" ;

static const char * line_shader_fragment_ = R"(

in vec4 colourV;
out vec4 fragColour;
void main(void) {
  fragColour = colourV ;
}
)";

void RendererImpl::drawLine(const Vector3f &from, const Vector3f &to, const Vector4f &clr, float lineWidth) {

    if ( !line_shader_ ) {
        line_shader_.reset(new OpenGLShaderProgram(line_shader_vertex_, line_shader_fragment_)) ;

        static const int MAX_POINTS_IN_BATCH  = 8 ;

        glGenVertexArrays(1, &line_vao_);
        glBindVertexArray(line_vao_);

        glGenBuffers(1, &line_vbo_);
        glGenBuffers(1, &line_idx_vbo_);

        glBindVertexArray(line_vao_);
        int sz = MAX_POINTS_IN_BATCH * sizeof(Vector3f);
        glBindBuffer(GL_ARRAY_BUFFER, line_vbo_);
        glBufferData(GL_ARRAY_BUFFER, sz, 0, GL_DYNAMIC_DRAW);

        glBindVertexArray(0);

        glGetIntegerv(GL_SMOOTH_LINE_WIDTH_RANGE, line_width_range_);
    }

    glEnable(GL_BLEND);
    // glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE);

    glEnable(GL_DEPTH_TEST) ;
    glDisable(GL_CULL_FACE) ;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);

    line_shader_->use() ;

    line_shader_->setUniform("mvp", Matrix4f(perspective_ * proj_) ) ;
    line_shader_->setUniform("colour", clr) ;

    const float vertexPositions[] = {
        from[0], from[1], from[2], 1,
        to[0], to[1], to[2], 1};

    int sz = sizeof(vertexPositions);

    glLineWidth(lineWidth);

    lineWidth = std::max((float)line_width_range_[0], std::min(lineWidth, (float)line_width_range_[1])) ;

    glBindVertexArray(line_vao_);

    glBindBuffer(GL_ARRAY_BUFFER, line_vbo_);

    {
        glBufferSubData(GL_ARRAY_BUFFER, 0, sz, vertexPositions);
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, line_vbo_);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);

    glDrawArrays(GL_LINES, 0, 2);

    glBindVertexArray(0);
    glLineWidth(1);
    glUseProgram(0);


}

}}}
