#include <cvx/viz/renderer/renderer.hpp>

#include "renderer_impl.hpp"

#include <iostream>
#include <cstring>

#include <Eigen/Dense>

#include <fstream>
#include <memory>

using namespace std ;
using namespace Eigen ;

namespace cvx { namespace viz {


void Renderer::setCamera(const CameraPtr &cam) {
    impl_->setCamera(cam) ;
}

void Renderer::render(const ScenePtr &scene) {
    impl_->renderShadowMap(scene, Vector3f{ 1, 1, 1 }) ;
    impl_->renderScene(scene) ;
}

Renderer::Renderer(int flags): impl_(new detail::RendererImpl(flags)) {
}

Renderer::~Renderer() {
}

cv::Mat Renderer::getColor(bool alpha) {
    return impl_->getColor(alpha) ;
}

cv::Mat Renderer::getColor(cv::Mat &bg, float alpha) {
    return impl_->getColor(bg, alpha) ;
}

cv::Mat Renderer::getDepth() {
    return impl_->getDepth() ;
}

void Renderer::clearZBuffer() {
    impl_->clearZBuffer();
}

void Renderer::text(const string &text, float x, float y, const Font &f, const Vector3f &clr)
{
    impl_->renderText(text, x, y, f, clr) ;
}

void Renderer::text(const string &text, const Vector3f &pos, const Font &f, const Vector3f &clr)
{
    impl_->renderText(text, pos, f, clr) ;
}

void Renderer::text(const Text &text, float x, float y, const Font &f, const Vector3f &clr)
{
    impl_->renderTextObject(text, x, y, f, clr) ;
}

void Renderer::text(const Text &text, const Vector3f &pos, const Font &f, const Vector3f &clr)
{
    impl_->renderTextObject(text, pos, f, clr) ;
}


Text Renderer::textObject(const string &text, const Font &f)
{
    return Text(text, f) ;
}

void Renderer::line(const Vector3f &from, const Vector3f &to, const Vector4f &clr, float lineWidth)
{
    impl_->drawLine(from, to, clr, lineWidth) ;
}

void Renderer::arc(const Vector3f& center, const Vector3f& normal, const Vector3f& axis, float radiusA, float radiusB,
                   float minAngle, float maxAngle, const Vector4f& color, bool drawSect, float lineWidth, float stepDegrees)
{
    const Vector3f& vx = axis;
    Vector3f vy = normal.cross(axis);
    float step = stepDegrees * M_PI/180.0;
    int nSteps = (int)fabs((maxAngle - minAngle) / step);
    if ( nSteps == 0 ) nSteps = 1;
    Vector3f prev = center + radiusA * vx * cos(minAngle) + radiusB * vy * sin(minAngle);

    if (drawSect)
        line(center, prev, color, lineWidth);

    for ( int i = 1; i <= nSteps; i++ )
    {
        float angle = minAngle + (maxAngle - minAngle) * i / float(nSteps);
        Vector3f next = center + radiusA * vx * cos(angle) + radiusB * vy * sin(angle);
        line(prev, next, color, lineWidth);
        prev = next;
    }

    if (drawSect)
        line(center, prev, color, lineWidth);
}

void Renderer::circle(const Vector3f &center, const Vector3f &normal, float radius, const Vector4f &color, float lineWidth)
{
    Vector3f axis(normal.y(), -normal.x(), normal.z()) ;
    arc(center, normal, axis, radius, radius, 0, 2*M_PI, color, false, lineWidth) ;
}

void Renderer::setDefaultFBO(uint fbo) {
    impl_->setDefaultFBO(fbo);
}


}}
