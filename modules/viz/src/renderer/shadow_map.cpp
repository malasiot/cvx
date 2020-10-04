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
     glBindFramebuffer(GL_FRAMEBUFFER, fbo_);

    // Create the depth buffer texture
    glGenTextures(1, &texture_id_);
    glBindTexture(GL_TEXTURE_2D, texture_id_);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, (GLsizei)width, (GLsizei)height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  //  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glBindTexture(GL_TEXTURE_2D, 0);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texture_id_, 0);

  // Disable writes to the color buffer
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
}


void ShadowMap::bind() {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
}

void ShadowMap::unbind(GLuint default_fbo = 0) {
    glBindFramebuffer(GL_FRAMEBUFFER, default_fbo);
}


void ShadowMap::bindTexture(GLenum textureUnit) {
    glActiveTexture(textureUnit);
    glBindTexture(GL_TEXTURE_2D, texture_id_);
}


}}}
