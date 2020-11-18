// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_SHELL_PLATFORM_COMMON_CPP_CLIENT_WRAPPER_INCLUDE_FLUTTER_TEXTURE_REGISTRAR_H_
#define FLUTTER_SHELL_PLATFORM_COMMON_CPP_CLIENT_WRAPPER_INCLUDE_FLUTTER_TEXTURE_REGISTRAR_H_

#include <flutter_texture_registrar.h>
#include <stdint.h>

#include <memory>
#include <variant>

namespace flutter {

// An interface used as an image source by texture widgets.
class PixelBufferTexture {
 public:
  virtual ~PixelBufferTexture() {}

  // Returns a FlutterDesktopPixelBuffer that contains the actual pixel data.
  // The intended surface size is specified by |width| and |height|.
  virtual const FlutterDesktopPixelBuffer* CopyPixelBuffer(size_t width,
                                                           size_t height) {
    return nullptr;
  };
};

typedef std::variant<PixelBufferTexture> TextureVariant;

// An object keeping track of external textures.
class TextureRegistrar {
 public:
  virtual ~TextureRegistrar() {}

  // Registers a |texture| object and returns the ID for that texture.
  virtual int64_t RegisterTexture(TextureVariant& texture) = 0;

  // Notifies the flutter engine that the texture object corresponding
  // to |texure_id| needs to render a new texture.
  virtual bool MarkTextureFrameAvailable(int64_t texture_id) = 0;

  // Unregisters an existing Texture object.
  virtual bool UnregisterTexture(int64_t texture_id) = 0;
};

}  // namespace flutter

#endif  // FLUTTER_SHELL_PLATFORM_COMMON_CPP_CLIENT_WRAPPER_INCLUDE_FLUTTER_TEXTURE_REGISTRAR_H_
