#include "shadow_map.hpp"
namespace cvx { namespace viz { namespace detail {

ShadowMap::~ShadowMap()
{
    if ( fbo_ )
        glDeleteFramebuffers(1, &fbo_);

    if ( texture_id_ )
        glDeleteTextures(1, &texture_id_);
}

bool ShadowMap::init(unsigned int width, unsigned int height)
{
    // Create the FBO
    glGenFramebuffers(1, &fbo_);

    // Create the depth buffer texture
    glGenTextures(1, &texture_id_);
    glBindTexture(GL_TEXTURE_2D, texture_id_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, (GLsizei)width, (GLsizei)height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texture_id_, 0);

    // Disable writes to the color buffer
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    if (  glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE )
        return false ;

    return true;
}


void ShadowMap::bind() {
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo_);
}

void ShadowMap::unbind() {
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}


void ShadowMap::bindTexture(GLenum textureUnit) {
    glActiveTexture(textureUnit);
    glBindTexture(GL_TEXTURE_2D, texture_id_);
}


}}}
