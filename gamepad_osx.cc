/*
 * Written by Simon Fuhrmann.
 * See LICENSE file for details.
 *
 * Some resources:
 * https://github.com/ThemsAllTook/libstem_gamepad/blob/master/source/gamepad/Gamepad_macosx.c
 * https://github.com/inolen/redream/blob/master/deps/sdl2/src/joystick/darwin/SDL_sysjoystick.c
 */
#ifdef __APPLE__

#include "gamepad_osx.h"

#include <chrono>
#include <thread>
#include <iostream>

#define RUN_LOOP_MODE_DISCOVERY CFSTR("RunLoopModeDiscovery")

namespace gamepad {
namespace {
constexpr int kHidPageDesktop = kHIDPage_GenericDesktop;
constexpr int kHidUsageGamepad = kHIDUsage_GD_GamePad;
constexpr int kHidUsageJoystick = kHIDUsage_GD_Joystick;
constexpr int kHidUsageController = kHIDUsage_GD_MultiAxisController;
}  // namespace

SystemImpl::SystemImpl() {
  pthread_mutex_init(&event_queue_mutex_, nullptr);
}

SystemImpl::~SystemImpl() {
  // Cancel event thread.
  if (event_thread_loop_ != nullptr) {
    pthread_cancel(event_thread_);
    event_thread_loop_ = nullptr;
  }
  pthread_mutex_destroy(&event_queue_mutex_);

  // Clean up devices.
  for (HidDevice* device : devices_) {
    HidCleanup(device);
    delete device;
  }
  devices_.clear();

  // Close event manager.
  if (hid_manager_ != nullptr) {
    IOHIDManagerUnscheduleFromRunLoop(hid_manager_, CFRunLoopGetCurrent(), kCFRunLoopCommonModes);
    IOHIDManagerClose(hid_manager_, kIOHIDOptionsTypeNone);
    CFRelease(hid_manager_);
    hid_manager_ = nullptr;
  }
}

void
SystemImpl::ProcessEvents() {
  // Process all events in the queue.
  HidProcessEvents();

  // Detach devices that have been removed.
  for (auto iter = devices_.begin(); iter != devices_.end();) {
    HidDevice* device = *iter;
    if (device->disconnected) {
      if (detached_handler_) {
        detached_handler_(&device->device);
      }
      delete device;
      iter = devices_.erase(iter);
    } else {
      iter++;
    }
  }
}

void
SystemImpl::ScanForDevices() {
  if (!initialized_) {
    HidInitialize();
  }

  // RunLoop for device discovery.
  CFRunLoopRunInMode(RUN_LOOP_MODE_DISCOVERY, /*seconds=*/0, true);
}

void
SystemImpl::HidInitialize() {
  // Create a new thread that receives input events.
  pthread_create(&event_thread_, nullptr, &SystemImpl::EventThreadRun, this);
  while (event_thread_loop_ == nullptr) {}

  // Create he HID manager.
  hid_manager_ = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
  if (hid_manager_ == nullptr) {
    std::cout << "Error creating HID manager." << std::endl;
    return;
  }

  // Create device matching dictionary.
  CFStringRef keys[2];
  keys[0] = CFSTR(kIOHIDDeviceUsagePageKey);
  keys[1] = CFSTR(kIOHIDDeviceUsageKey);

  CFDictionaryRef dictionaries[3];
  CFNumberRef values[2];
	values[0] = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &kHidPageDesktop);
	values[1] = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &kHidUsageJoystick);
	dictionaries[0] = CFDictionaryCreate(kCFAllocatorDefault, (const void **)keys, (const void **)values, 2, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
  CFRelease(values[0]);
  CFRelease(values[1]);

	values[0] = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &kHidPageDesktop);
	values[1] = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &kHidUsageGamepad);
	dictionaries[1] = CFDictionaryCreate(kCFAllocatorDefault, (const void **)keys, (const void **)values, 2, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
  CFRelease(values[0]);
  CFRelease(values[1]);

	values[0] = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &kHidPageDesktop);
	values[1] = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &kHidUsageController);
	dictionaries[2] = CFDictionaryCreate(kCFAllocatorDefault, (const void **)keys, (const void **)values, 2, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
  CFRelease(values[0]);
  CFRelease(values[1]);

  CFArrayRef dictionariesRef = CFArrayCreate(kCFAllocatorDefault,
      (const void **)dictionaries, 3, &kCFTypeArrayCallBacks);
  CFRelease(dictionaries[0]);
	CFRelease(dictionaries[1]);
	CFRelease(dictionaries[2]);

  // Set the dictionary.
  IOHIDManagerSetDeviceMatchingMultiple(hid_manager_, dictionariesRef);
  CFRelease(dictionariesRef);

  // Register attached and detached callbacks.
  IOHIDManagerRegisterDeviceMatchingCallback(hid_manager_, SystemImpl::HidAttached, this);
	IOHIDManagerRegisterDeviceRemovalCallback(hid_manager_, SystemImpl::HidDetached, this);

  // Open the HID manager.
  if (IOHIDManagerOpen(hid_manager_, kIOHIDOptionsTypeNone) != kIOReturnSuccess) {
    std::cout << "Error opening HID manager." << std::endl;
    return;
  }

  // Process initial events.
  IOHIDManagerScheduleWithRunLoop(hid_manager_, CFRunLoopGetCurrent(), RUN_LOOP_MODE_DISCOVERY);
  initialized_ = true;
}

void
SystemImpl::HidCleanup(HidDevice* device) {
  device->disconnected = true;
  if (device->device_ref != nullptr) {
    IOHIDDeviceClose(device->device_ref, kIOHIDOptionsTypeNone);
    device->device_ref = nullptr;
  }
}

void
SystemImpl::HidAttached(void* context, IOReturn result, void* sender, IOHIDDeviceRef device) {
  SystemImpl* system = static_cast<SystemImpl*>(context);
  system->HidDeviceAttached(device);
}

void
SystemImpl::HidDetached(void* context, IOReturn result, void* sender, IOHIDDeviceRef device) {
  SystemImpl* system = static_cast<SystemImpl*>(context);
  system->HidDeviceDetached(device);
}

void
SystemImpl::HidInput(void* context, IOReturn result, void* sender, IOHIDValueRef value) {
  HidDevice* device = static_cast<HidDevice*>(context);
  device->parent->HidDeviceInput(device, value);
}

void*
SystemImpl::EventThreadRun(void* context) {
  SystemImpl* system = static_cast<SystemImpl*>(context);
  system->HidEventThread();
  return nullptr;
}

void
SystemImpl::HidDeviceAttached(IOHIDDeviceRef device) {
  // Get vendor and product ID.
  CFTypeRef vendorRef = IOHIDDeviceGetProperty(device, CFSTR(kIOHIDVendorIDKey));
  CFTypeRef productRef = IOHIDDeviceGetProperty(device, CFSTR(kIOHIDVendorIDKey));
  if (vendorRef == nullptr || productRef == nullptr ||
      CFGetTypeID(vendorRef) != CFNumberGetTypeID() ||
      CFGetTypeID(productRef) != CFNumberGetTypeID()) {
    std::cerr << "Error: Vendor or Product ID not numbers!" << std::endl;
    return;
  }
  int vendor_id, product_id;
  CFNumberGetValue((CFNumberRef)vendorRef, kCFNumberSInt32Type, &vendor_id);
  CFNumberGetValue((CFNumberRef)productRef, kCFNumberSInt32Type, &product_id);

  // Get device name.
  std::string device_name;
  CFTypeRef nameRef = IOHIDDeviceGetProperty(device, CFSTR(kIOHIDProductKey));
	if (nameRef == nullptr || CFGetTypeID(nameRef) != CFStringGetTypeID()) {
    device_name = "<Unknown>";
  } else {
    char buffer[1024];
    CFStringGetCString((CFStringRef)nameRef, buffer, 1024, kCFStringEncodingUTF8);
    device_name = buffer;
  }

  // Create the device record.
  HidDevice* hid_device = new HidDevice();
  hid_device->device_ref = device;
  hid_device->parent = this;
  hid_device->device.vendor_id = vendor_id;
  hid_device->device.product_id = product_id;
  hid_device->device.description = device_name;

  // Scan buttons and axes.
  int max_cookie_id = 0;
  CFArrayRef elements = IOHIDDeviceCopyMatchingElements(device, nullptr, kIOHIDOptionsTypeNone);
	for (int i = 0; i < CFArrayGetCount(elements); i++) {
		IOHIDElementRef element = (IOHIDElementRef)CFArrayGetValueAtIndex(elements, i);
		IOHIDElementType type = IOHIDElementGetType(element);

    const uint32_t usagePage = IOHIDElementGetUsagePage(element);
    //const uint32_t usage = IOHIDElementGetUsage(element);
    if (usagePage != kHIDPage_GenericDesktop &&
        usagePage != kHIDPage_Button) {
      continue;
    }

    if (type == kIOHIDElementTypeInput_Button) {
      HidButtonInfo button_info;
      button_info.button_id = hid_device->button_infos.size();
      button_info.cookie = IOHIDElementGetCookie(element);
      hid_device->button_infos.push_back(button_info);
      max_cookie_id = std::max(max_cookie_id, button_info.cookie);
		} else if (type == kIOHIDElementTypeInput_Misc ||
        type == kIOHIDElementTypeInput_Axis) {
      HidAxisInfo axis_info;
      axis_info.axis_id = hid_device->axis_infos.size();
      axis_info.cookie = IOHIDElementGetCookie(element);
      axis_info.minimum = IOHIDElementGetLogicalMin(element);
      axis_info.maximum = IOHIDElementGetLogicalMax(element);
      hid_device->axis_infos.push_back(axis_info);
      max_cookie_id = std::max(max_cookie_id, axis_info.cookie);
    }
	}
	CFRelease(elements);

  // Initialize button and axis lists.
  hid_device->device.buttons.resize(hid_device->button_infos.size(), false);
  hid_device->device.axes.resize(hid_device->axis_infos.size(), 0.0f);

  // Create a lookup table from cookie ID to axis/button ID.
  hid_device->cookie_map.resize(max_cookie_id + 1);
  for (const HidButtonInfo& button : hid_device->button_infos) {
    hid_device->cookie_map[button.cookie].button_id = button.button_id;
  }
  for (const HidAxisInfo& axis : hid_device->axis_infos) {
    hid_device->cookie_map[axis.cookie].axis_id = axis.axis_id;
  }

  // Assign device ID and notify client.
  hid_device->device.device_id = next_device_id_++;
  devices_.push_back(hid_device);
  if (attached_handler_) {
    attached_handler_(&devices_.back()->device);
  }

  // Open HID device and attach input callback.
  IOHIDDeviceOpen(device, kIOHIDOptionsTypeNone);
  IOHIDDeviceRegisterInputValueCallback(device, SystemImpl::HidInput, devices_.back());
  // Schedule event handling on a separate thread.
  IOHIDDeviceScheduleWithRunLoop(device, event_thread_loop_, kCFRunLoopDefaultMode);
}

void
SystemImpl::HidDeviceDetached(IOHIDDeviceRef device) {
  // De-allocate existing devices, fire callback later on ProcessEvents().
  for (HidDevice* hid_device : devices_) {
    if (hid_device->device_ref == device) {
      HidCleanup(hid_device);
      return;
    }
  }
}

void
SystemImpl::HidDeviceInput(HidDevice* hid_device, IOHIDValueRef value) {
  if (hid_device == nullptr || value == nullptr) return;

  IOHIDElementRef element = IOHIDValueGetElement(value);
  IOHIDElementCookie cookie = IOHIDElementGetCookie(element);
  const int int_value = IOHIDValueGetIntegerValue(value);

  // Reject events with cookies that are not mapped.
  if (cookie >= hid_device->cookie_map.size()) {
    return;
  }
  const HidCookieInfo& cookie_info = hid_device->cookie_map[cookie];
  if (cookie_info.axis_id < 0 && cookie_info.button_id < 0) {
    return;
  }

  HidEvent event;
  event.device = hid_device;
  event.axis_id = cookie_info.axis_id;
  event.button_id = cookie_info.button_id;
  event.value = int_value;

  pthread_mutex_lock(&event_queue_mutex_);
  // Queue the event for processing in the main thread.
  event_queue_.push(event);
  // If the event queue has grown too large, compress the queue.
  if (event_queue_.size() > 1024) {
    HidCompressQueue();
  }
  pthread_mutex_unlock(&event_queue_mutex_);
}

void
SystemImpl::HidCompressQueue() {
  std::cout << "Compressing event queue (size " << event_queue_.size()
      << ")..." << std::endl;
  pthread_mutex_lock(&event_queue_mutex_);
  // TODO
  while (!event_queue_.empty())
    event_queue_.pop();
  pthread_mutex_unlock(&event_queue_mutex_); 
}

void
SystemImpl::HidProcessEvents() {
  // Limit event processing to the events currently in the queue. This
  // potentially prevents running this function forever if event generation is
  // faster than processing.
  pthread_mutex_lock(&event_queue_mutex_);
  std::size_t num_events = event_queue_.size();
  pthread_mutex_unlock(&event_queue_mutex_);
  while (num_events > 0) {
    // Get an event from the queue and decrement the number of events that
    // still need to be processed. Since event queue compression may happen in
    // the background if the queue gets too long, limit by actual queue size.
    // Compression will always leave at least one element in the queue.
    pthread_mutex_lock(&event_queue_mutex_);
    HidEvent event = event_queue_.front();
    event_queue_.pop();
    num_events = std::min(num_events - 1, event_queue_.size());
    pthread_mutex_unlock(&event_queue_mutex_);
    HidProcessEvent(event);
  }
}

void
SystemImpl::HidProcessEvent(const HidEvent& event) {
  HidDevice* device = event.device;
  if (event.button_id >= 0) {
    const HidButtonInfo& button_info = device->button_infos[event.button_id];
    HandleButtonEvent(&device->device, event.button_id, event.value);
  } else if (event.axis_id >= 0) {
    const HidAxisInfo& axis_info = device->axis_infos[event.axis_id];
    HandleAxisEvent(&device->device, event.axis_id, event.value,
        axis_info.minimum, axis_info.maximum, axis_info.fuzz, axis_info.flat);
  }
}

void
SystemImpl::HidEventThread() {
  // Export this thread's run loop.
  event_thread_loop_ = CFRunLoopGetCurrent();

  // Run the run loop indefinitely. CFRunLoopRun() must be called in a loop
  // as CFRunLoopRun() will return if it does not contain any sources (i.e.,
  // when no devices are attached). Sleep to not busy-loop.
  while (true) {
    CFRunLoopRun();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}

}  // namespace gamepad

#endif  // __APPLE__
