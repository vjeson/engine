// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <map>
#include <memory>
#include <vector>

#include "flutter/shell/platform/common/cpp/client_wrapper/include/flutter/plugin_registrar.h"
#include "flutter/shell/platform/common/cpp/client_wrapper/include/flutter/texture_registrar.h"
#include "flutter/shell/platform/common/cpp/client_wrapper/testing/stub_flutter_api.h"
#include "gtest/gtest.h"

namespace flutter {

namespace {

// Stub implementation to validate calls to the API.
class TestApi : public testing::StubFlutterApi {
 public:
  struct FakeTexture {
    int64_t texture_id;
    int32_t mark_count;
    FlutterDesktopTextureCallback texture_callback;
    void* user_data;
  };

 public:
  // |flutter::testing::StubFlutterApi|
  bool MessengerSend(const char* channel,
                     const uint8_t* message,
                     const size_t message_size) override {
    last_data_sent_ = message;
    return message_engine_result;
  }

  bool MessengerSendWithReply(const char* channel,
                              const uint8_t* message,
                              const size_t message_size,
                              const FlutterDesktopBinaryReply reply,
                              void* user_data) override {
    last_data_sent_ = message;
    return message_engine_result;
  }

  void MessengerSetCallback(const char* channel,
                            FlutterDesktopMessageCallback callback,
                            void* user_data) override {
    last_message_callback_set_ = callback;
  }

  void RegistrarSetDestructionHandler(
      FlutterDesktopOnRegistrarDestroyed callback) override {
    last_destruction_callback_set_ = callback;
  }

  const uint8_t* last_data_sent() { return last_data_sent_; }
  FlutterDesktopMessageCallback last_message_callback_set() {
    return last_message_callback_set_;
  }
  FlutterDesktopOnRegistrarDestroyed last_destruction_callback_set() {
    return last_destruction_callback_set_;
  }

  int64_t RegisterExternalTexture(
      FlutterDesktopTextureCallback texture_callback,
      void* user_data) override {
    last_texture_id_++;

    auto texture = std::make_unique<FakeTexture>();
    texture->texture_callback = texture_callback;
    texture->user_data = user_data;
    texture->mark_count = 0;
    texture->texture_id = last_texture_id_;

    textures_[last_texture_id_] = std::move(texture);
    return last_texture_id_;
  }

  bool UnregisterExternalTexture(int64_t texture_id) override {
    auto it = textures_.find(texture_id);
    if (it != textures_.end()) {
      textures_.erase(it);
      return true;
    }
    return false;
  }

  bool TextureFrameAvailable(int64_t texture_id) override {
    auto it = textures_.find(texture_id);
    if (it != textures_.end()) {
      it->second->mark_count++;
      return true;
    }
    return false;
  }

  FakeTexture* GetFakeTexture(int64_t texture_id) {
    auto it = textures_.find(texture_id);
    if (it != textures_.end()) {
      return it->second.get();
    }
    return nullptr;
  }

  int64_t last_texture_id() { return last_texture_id_; }

  size_t textures_size() { return textures_.size(); }

 private:
  const uint8_t* last_data_sent_ = nullptr;
  FlutterDesktopMessageCallback last_message_callback_set_ = nullptr;
  FlutterDesktopOnRegistrarDestroyed last_destruction_callback_set_ = nullptr;
  int64_t last_texture_id_ = -1;
  std::map<int64_t, std::unique_ptr<FakeTexture>> textures_;
};

// A PluginRegistrar whose destruction can be watched for by tests.
class TestPluginRegistrar : public PluginRegistrar {
 public:
  explicit TestPluginRegistrar(FlutterDesktopPluginRegistrarRef core_registrar)
      : PluginRegistrar(core_registrar) {}

  virtual ~TestPluginRegistrar() {
    if (destruction_callback_) {
      destruction_callback_();
    }
  }

  void SetDestructionCallback(std::function<void()> callback) {
    destruction_callback_ = std::move(callback);
  }

 private:
  std::function<void()> destruction_callback_;
};

}  // namespace

// Tests that the registrar returns a messenger that passes Send through to the
// C API.
TEST(PluginRegistrarTest, MessengerSend) {
  testing::ScopedStubFlutterApi scoped_api_stub(std::make_unique<TestApi>());
  auto test_api = static_cast<TestApi*>(scoped_api_stub.stub());

  auto dummy_registrar_handle =
      reinterpret_cast<FlutterDesktopPluginRegistrarRef>(1);
  PluginRegistrar registrar(dummy_registrar_handle);
  BinaryMessenger* messenger = registrar.messenger();

  std::vector<uint8_t> message = {1, 2, 3, 4};
  messenger->Send("some_channel", &message[0], message.size());
  EXPECT_EQ(test_api->last_data_sent(), &message[0]);
}

// Tests that the registrar returns a messenger that passes callback
// registration and unregistration through to the C API.
TEST(PluginRegistrarTest, MessengerSetMessageHandler) {
  testing::ScopedStubFlutterApi scoped_api_stub(std::make_unique<TestApi>());
  auto test_api = static_cast<TestApi*>(scoped_api_stub.stub());

  auto dummy_registrar_handle =
      reinterpret_cast<FlutterDesktopPluginRegistrarRef>(1);
  PluginRegistrar registrar(dummy_registrar_handle);
  BinaryMessenger* messenger = registrar.messenger();
  const std::string channel_name("foo");

  // Register.
  BinaryMessageHandler binary_handler = [](const uint8_t* message,
                                           const size_t message_size,
                                           BinaryReply reply) {};
  messenger->SetMessageHandler(channel_name, std::move(binary_handler));
  EXPECT_NE(test_api->last_message_callback_set(), nullptr);

  // Unregister.
  messenger->SetMessageHandler(channel_name, nullptr);
  EXPECT_EQ(test_api->last_message_callback_set(), nullptr);
}

// Tests that the registrar manager returns the same instance when getting
// the wrapper for the same reference.
TEST(PluginRegistrarTest, ManagerSameInstance) {
  PluginRegistrarManager* manager = PluginRegistrarManager::GetInstance();
  manager->Reset();

  testing::ScopedStubFlutterApi scoped_api_stub(std::make_unique<TestApi>());

  auto dummy_registrar_handle =
      reinterpret_cast<FlutterDesktopPluginRegistrarRef>(1);

  EXPECT_EQ(manager->GetRegistrar<PluginRegistrar>(dummy_registrar_handle),
            manager->GetRegistrar<PluginRegistrar>(dummy_registrar_handle));
}

// Tests that the registrar manager returns different objects for different
// references.
TEST(PluginRegistrarTest, ManagerDifferentInstances) {
  PluginRegistrarManager* manager = PluginRegistrarManager::GetInstance();
  manager->Reset();

  testing::ScopedStubFlutterApi scoped_api_stub(std::make_unique<TestApi>());

  auto dummy_registrar_handle_a =
      reinterpret_cast<FlutterDesktopPluginRegistrarRef>(1);
  auto dummy_registrar_handle_b =
      reinterpret_cast<FlutterDesktopPluginRegistrarRef>(2);

  EXPECT_NE(manager->GetRegistrar<PluginRegistrar>(dummy_registrar_handle_a),
            manager->GetRegistrar<PluginRegistrar>(dummy_registrar_handle_b));
}

// Tests that the registrar manager deletes wrappers when the underlying
// reference is destroyed.
TEST(PluginRegistrarTest, ManagerRemovesOnDestruction) {
  PluginRegistrarManager* manager = PluginRegistrarManager::GetInstance();
  manager->Reset();

  testing::ScopedStubFlutterApi scoped_api_stub(std::make_unique<TestApi>());
  auto test_api = static_cast<TestApi*>(scoped_api_stub.stub());

  auto dummy_registrar_handle =
      reinterpret_cast<FlutterDesktopPluginRegistrarRef>(1);
  auto* wrapper =
      manager->GetRegistrar<TestPluginRegistrar>(dummy_registrar_handle);

  // Simulate destruction of the reference, and ensure that the wrapper
  // is destroyed.
  EXPECT_NE(test_api->last_destruction_callback_set(), nullptr);
  bool destroyed = false;
  wrapper->SetDestructionCallback([&destroyed]() { destroyed = true; });
  test_api->last_destruction_callback_set()(dummy_registrar_handle);
  EXPECT_EQ(destroyed, true);

  // Requesting the wrapper should now create a new object.
  EXPECT_NE(manager->GetRegistrar<TestPluginRegistrar>(dummy_registrar_handle),
            nullptr);
}

// Tests texture register that calls through to the C API.
TEST(MethodCallTest, RegisterTexture) {
  testing::ScopedStubFlutterApi scoped_api_stub(std::make_unique<TestApi>());
  auto test_api = static_cast<TestApi*>(scoped_api_stub.stub());

  auto dummy_registrar_handle =
      reinterpret_cast<FlutterDesktopPluginRegistrarRef>(1);
  PluginRegistrar registrar(dummy_registrar_handle);
  TextureRegistrar* textures = registrar.textures();

  EXPECT_EQ(test_api->last_texture_id(), -1);
  auto texture = test_api->GetFakeTexture(0);
  EXPECT_EQ(texture, nullptr);

  int64_t texture_id = textures->RegisterTexture(reinterpret_cast<Texture*>(2));
  EXPECT_EQ(test_api->last_texture_id(), texture_id);
  EXPECT_EQ(test_api->textures_size(), static_cast<size_t>(1));

  texture = test_api->GetFakeTexture(texture_id);
  EXPECT_EQ(texture->texture_id, texture_id);
  EXPECT_EQ(texture->user_data, reinterpret_cast<Texture*>(2));

  textures->MarkTextureFrameAvailable(texture_id);
  textures->MarkTextureFrameAvailable(texture_id);
  textures->MarkTextureFrameAvailable(texture_id);
  EXPECT_EQ(texture->mark_count, 3);

  textures->UnregisterTexture(texture_id);
  texture = test_api->GetFakeTexture(texture_id);
  EXPECT_EQ(texture, nullptr);
  EXPECT_EQ(test_api->textures_size(), static_cast<size_t>(0));
}

}  // namespace flutter
