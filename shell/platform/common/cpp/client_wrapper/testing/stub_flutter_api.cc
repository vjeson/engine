// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/common/cpp/client_wrapper/testing/stub_flutter_api.h"

static flutter::testing::StubFlutterApi* s_stub_implementation;

namespace flutter {
namespace testing {

// static
void StubFlutterApi::SetTestStub(StubFlutterApi* stub) {
  s_stub_implementation = stub;
}

// static
StubFlutterApi* StubFlutterApi::GetTestStub() {
  return s_stub_implementation;
}

ScopedStubFlutterApi::ScopedStubFlutterApi(std::unique_ptr<StubFlutterApi> stub)
    : stub_(std::move(stub)) {
  previous_stub_ = StubFlutterApi::GetTestStub();
  StubFlutterApi::SetTestStub(stub_.get());
}

ScopedStubFlutterApi::~ScopedStubFlutterApi() {
  StubFlutterApi::SetTestStub(previous_stub_);
}

}  // namespace testing
}  // namespace flutter

// Forwarding dummy implementations of the C API.

FlutterDesktopMessengerRef FlutterDesktopRegistrarGetMessenger(
    FlutterDesktopPluginRegistrarRef registrar) {
  // The stub ignores this, so just return an arbitrary non-zero value.
  return reinterpret_cast<FlutterDesktopMessengerRef>(1);
}

void FlutterDesktopRegistrarSetDestructionHandler(
    FlutterDesktopPluginRegistrarRef registrar,
    FlutterDesktopOnRegistrarDestroyed callback) {
  if (s_stub_implementation) {
    s_stub_implementation->RegistrarSetDestructionHandler(callback);
  }
}

bool FlutterDesktopMessengerSend(FlutterDesktopMessengerRef messenger,
                                 const char* channel,
                                 const uint8_t* message,
                                 const size_t message_size) {
  bool result = false;
  if (s_stub_implementation) {
    result =
        s_stub_implementation->MessengerSend(channel, message, message_size);
  }
  return result;
}

bool FlutterDesktopMessengerSendWithReply(FlutterDesktopMessengerRef messenger,
                                          const char* channel,
                                          const uint8_t* message,
                                          const size_t message_size,
                                          const FlutterDesktopBinaryReply reply,
                                          void* user_data) {
  bool result = false;
  if (s_stub_implementation) {
    result = s_stub_implementation->MessengerSendWithReply(
        channel, message, message_size, reply, user_data);
  }
  return result;
}

void FlutterDesktopMessengerSendResponse(
    FlutterDesktopMessengerRef messenger,
    const FlutterDesktopMessageResponseHandle* handle,
    const uint8_t* data,
    size_t data_length) {
  if (s_stub_implementation) {
    s_stub_implementation->MessengerSendResponse(handle, data, data_length);
  }
}

void FlutterDesktopMessengerSetCallback(FlutterDesktopMessengerRef messenger,
                                        const char* channel,
                                        FlutterDesktopMessageCallback callback,
                                        void* user_data) {
  if (s_stub_implementation) {
    s_stub_implementation->MessengerSetCallback(channel, callback, user_data);
  }
}

FlutterDesktopTextureRegistrarRef FlutterDesktopRegistrarGetTextureRegistrar(
    FlutterDesktopPluginRegistrarRef registrar) {
  return reinterpret_cast<FlutterDesktopTextureRegistrarRef>(1);
}

int64_t FlutterDesktopRegisterExternalTexture(
    FlutterDesktopTextureRegistrarRef texture_registrar,
    FlutterDesktopTextureCallback texture_callback,
    void* user_data) {
  uint64_t result = -1;
  if (s_stub_implementation) {
    result = s_stub_implementation->RegisterExternalTexture(texture_callback,
                                                            user_data);
  }
  return result;
}

bool FlutterDesktopUnregisterExternalTexture(
    FlutterDesktopTextureRegistrarRef texture_registrar,
    int64_t texture_id) {
  bool result = false;
  if (s_stub_implementation) {
    result = s_stub_implementation->UnregisterExternalTexture(texture_id);
  }
  return result;
}

bool FlutterDesktopMarkExternalTextureFrameAvailable(
    FlutterDesktopTextureRegistrarRef texture_registrar,
    int64_t texture_id) {
  bool result = false;
  if (s_stub_implementation) {
    result = s_stub_implementation->TextureFrameAvailable(texture_id);
  }
  return result;
}
