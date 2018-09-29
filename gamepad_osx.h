/*
 * Written by Simon Fuhrmann.
 * See LICENSE file for details.
 */
#ifndef GAMEPAD_OSX_HEADER
#define GAMEPAD_OSX_HEADER
#ifdef __APPLE__

#include <pthread.h>
#include <CoreFoundation/CFRunLoop.h>
#include <IOKit/hid/IOHIDManager.h>
#include <vector>
#include <queue>

#include "gamepad.h"

namespace gamepad {

class SystemImpl;

struct HidButtonInfo {
  int button_id = -1;
  int cookie = -1;
};

struct HidAxisInfo {
  int axis_id = -1;
  int cookie = -1;
  int minimum = 0;
  int maximum = 0;
  int fuzz = 0;
  int flat = 0;
};

struct HidCookieInfo {
  int axis_id = -1;
  int button_id = -1;
};

struct HidDevice {
  IOHIDDeviceRef device_ref = nullptr;
  SystemImpl* parent = nullptr;
  bool disconnected = false;
  Device device;
  std::vector<HidButtonInfo> button_infos;
  std::vector<HidAxisInfo> axis_infos;
  std::vector<HidCookieInfo> cookie_map;
};

struct HidEvent {
  HidDevice* device = nullptr;
  int axis_id = -1;
  int button_id = -1;
  int value;
};

class SystemImpl : public System {
 public:
  SystemImpl();
  ~SystemImpl() override;
  void ProcessEvents() override;
  void ScanForDevices() override;

 private:
  void HidInitialize();
  void HidCleanup(HidDevice* device);
  void HidDeviceAttached(IOHIDDeviceRef device);
  void HidDeviceDetached(IOHIDDeviceRef device);
  void HidDeviceInput(HidDevice* hid_device, IOHIDValueRef value);
  void HidEventThread();
  void HidProcessEvents();
  void HidProcessEvent(const HidEvent& event);
  void HidCompressQueue();

  static void HidAttached(
      void* context, IOReturn result, void* sender, IOHIDDeviceRef device);
  static void HidDetached(
      void* context, IOReturn result, void* sender, IOHIDDeviceRef device);
  static void HidInput(
      void* context, IOReturn result, void* sender, IOHIDValueRef value);
  static void* EventThreadRun(void* context);

 private:
  bool initialized_ = false;
  int next_device_id_ = 0;
  IOHIDManagerRef hid_manager_ = nullptr;
  std::vector<HidDevice*> devices_;

  pthread_t event_thread_;
  CFRunLoopRef event_thread_loop_ = nullptr;
  std::queue<HidEvent> event_queue_;
  pthread_mutex_t event_queue_mutex_;
};

}  // namespace gamepad

#endif  // __APPLE__
#endif  // GAMEPAD_OSX_HEADER
