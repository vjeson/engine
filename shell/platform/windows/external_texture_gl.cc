// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/windows/external_texture_gl.h"

// OpenGL ES and EGL includes
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <EGL/eglplatform.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <iostream>

namespace flutter {

typedef void (*glGenTexturesProc)(GLsizei n, GLuint* textures);
typedef void (*glDeleteTexturesProc)(GLsizei n, const GLuint* textures);
typedef void (*glBindTextureProc)(GLenum target, GLuint texture);
typedef void (*glTexParameteriProc)(GLenum target, GLenum pname, GLint param);
typedef void (*glTexImage2DProc)(GLenum target,
                                 GLint level,
                                 GLint internalformat,
                                 GLsizei width,
                                 GLsizei height,
                                 GLint border,
                                 GLenum format,
                                 GLenum type,
                                 const void* data);

struct GlProcs {
  explicit GlProcs() {
    glGenTextures =
        reinterpret_cast<glGenTexturesProc>(eglGetProcAddress("glGenTextures"));
    glDeleteTextures = reinterpret_cast<glDeleteTexturesProc>(
        eglGetProcAddress("glDeleteTextures"));
    glBindTexture =
        reinterpret_cast<glBindTextureProc>(eglGetProcAddress("glBindTexture"));
    glTexParameteri = reinterpret_cast<glTexParameteriProc>(
        eglGetProcAddress("glTexParameteri"));
    glTexImage2D =
        reinterpret_cast<glTexImage2DProc>(eglGetProcAddress("glTexImage2D"));

    valid = glGenTextures && glDeleteTextures && glBindTexture &&
            glTexParameteri && glTexImage2D;
  }

  glGenTexturesProc glGenTextures;
  glDeleteTexturesProc glDeleteTextures;
  glBindTextureProc glBindTexture;
  glTexParameteriProc glTexParameteri;
  glTexImage2DProc glTexImage2D;
  bool valid;
};

struct ExternalTextureGLState {
  GLuint gl_texture = 0;
  static const GlProcs procs;
};

const GlProcs ExternalTextureGLState::procs = []() {
  GlProcs procs;
  return procs;
}();

ExternalTextureGL::ExternalTextureGL(
    FlutterDesktopTextureCallback texture_callback,
    void* user_data)
    : state_(std::make_unique<ExternalTextureGLState>()),
      texture_callback_(texture_callback),
      user_data_(user_data) {}

ExternalTextureGL::~ExternalTextureGL() {
  const auto& procs = state_->procs;

  if (procs.valid && state_->gl_texture != 0) {
    procs.glDeleteTextures(1, &state_->gl_texture);
  }
}

bool ExternalTextureGL::PopulateTextureWithIdentifier(
    size_t width,
    size_t height,
    FlutterOpenGLTexture* opengl_texture) {
  const FlutterDesktopPixelBuffer* pixel_buffer =
      texture_callback_(width, height, user_data_);

  const auto& procs = state_->procs;

  if (!procs.valid || !pixel_buffer || !pixel_buffer->buffer) {
    std::cerr << "Failed to copy pixel buffer from plugin." << std::endl;
    return false;
  }

  if (state_->gl_texture == 0) {
    procs.glGenTextures(1, &state_->gl_texture);

    procs.glBindTexture(GL_TEXTURE_2D, state_->gl_texture);

    procs.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    procs.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    procs.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    procs.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  } else {
    procs.glBindTexture(GL_TEXTURE_2D, state_->gl_texture);
  }

  procs.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, pixel_buffer->width,
                     pixel_buffer->height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                     pixel_buffer->buffer);

  opengl_texture->target = GL_TEXTURE_2D;
  opengl_texture->name = state_->gl_texture;
  opengl_texture->format = GL_RGBA8;
  opengl_texture->destruction_callback = (VoidCallback) nullptr;
  opengl_texture->user_data = static_cast<void*>(this);
  opengl_texture->width = pixel_buffer->width;
  opengl_texture->height = pixel_buffer->height;

  return true;
}

}  // namespace flutter
