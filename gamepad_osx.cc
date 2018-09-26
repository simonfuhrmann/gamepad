#ifdef __APPLE__

#include "gamepad_osx.h"

#include <iostream>

#define RUN_LOOP_MODE_DISCOVERY CFSTR("RunLoopModeDiscovery")
#define RUN_LOOP_MODE_INPUT CFSTR("RunLoopModeInput")

namespace gamepad {
namespace {
constexpr int kHidPageDesktop = kHIDPage_GenericDesktop;
constexpr int kHidUsageGamepad = kHIDUsage_GD_GamePad;
constexpr int kHidUsageJoystick = kHIDUsage_GD_Joystick;
constexpr int kHidUsageController = kHIDUsage_GD_MultiAxisController;
}  // namespace

SystemImpl::~SystemImpl() {
  for (HidDevice& device : devices_) {
    HidCleanup(&device);
  }
  if (hid_manager_ != nullptr) {
    IOHIDManagerUnscheduleFromRunLoop(hid_manager_, CFRunLoopGetCurrent(), kCFRunLoopCommonModes);
    IOHIDManagerClose(hid_manager_, kIOHIDOptionsTypeNone);
    CFRelease(hid_manager_);
    hid_manager_ = nullptr;
  }
}

void
SystemImpl::ProcessEvents() {
  if (!initialized_) {
    HidInitialize();
  }

  // RunLoop for event handling. This processes more than one event, and
  // calling this in a loop does not seem to make a difference. Events can get
  // swallowed either way if the device issues lots of input events and the
  // polling interval is too long.
  CFRunLoopRunInMode(RUN_LOOP_MODE_INPUT, /*seconds=*/0, true);

  // Detach devices that have been removed.
  for (auto iter = devices_.begin(); iter != devices_.end();) {
    if (iter->disconnected) {
      if (detached_handler_) {
        detached_handler_(&iter->device);
      }
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
  SystemImpl* system = static_cast<SystemImpl*>(context);
  system->HidDeviceInput(value);
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
  HidDevice hid_device;
  hid_device.device_ref = device;
  hid_device.device.vendor_id = vendor_id;
  hid_device.device.product_id = product_id;
  hid_device.device.description = device_name;

  // Scan buttons and axes.
  CFArrayRef elements = IOHIDDeviceCopyMatchingElements(device, nullptr, kIOHIDOptionsTypeNone);
	for (int i = 0; i < CFArrayGetCount(elements); i++) {
		IOHIDElementRef element = (IOHIDElementRef)CFArrayGetValueAtIndex(elements, i);
		IOHIDElementType type = IOHIDElementGetType(element);

    if (type == kIOHIDElementTypeInput_Button) {
      HidButtonInfo button_info;
      button_info.button_id = hid_device.button_map.size();
      button_info.cookie = IOHIDElementGetCookie(element);
      hid_device.button_map.push_back(button_info);
		} else if (type == kIOHIDElementTypeInput_Misc ||
        type == kIOHIDElementTypeInput_Axis) {
      if (hid_device.axis_map.size() > 8) continue;  // TODO: Fix for PS4
      HidAxisInfo axis_info;
      axis_info.axis_id = hid_device.axis_map.size();
      axis_info.cookie = IOHIDElementGetCookie(element);
      axis_info.minimum = IOHIDElementGetLogicalMin(element);
      axis_info.maximum = IOHIDElementGetLogicalMax(element);
      hid_device.axis_map.push_back(axis_info);
    }
	}
	CFRelease(elements);

  // Initialize button and axis lists.
  hid_device.device.buttons.resize(hid_device.button_map.size(), false);
  hid_device.device.axes.resize(hid_device.axis_map.size(), 0.0f);

  // Assign device ID and notify client.
  hid_device.device.device_id = next_device_id_++;
  devices_.push_back(hid_device);
  if (attached_handler_) {
    attached_handler_(&devices_.back().device);
  }

  // Open HID device and attach input callback.
  IOHIDDeviceOpen(device, kIOHIDOptionsTypeNone);
  IOHIDDeviceScheduleWithRunLoop(device, CFRunLoopGetCurrent(), RUN_LOOP_MODE_INPUT);
  IOHIDDeviceRegisterInputValueCallback(device, SystemImpl::HidInput, this);
}

void
SystemImpl::HidDeviceDetached(IOHIDDeviceRef device) {
  // De-allocate existing devices, fire callback later on ProcessEvents().
  for (HidDevice& hid_device : devices_) {
    if (hid_device.device_ref == device) {
      HidCleanup(&hid_device);
      return;
    }
  }
}

void
SystemImpl::HidDeviceInput(IOHIDValueRef value) {
  if (!value) return;
  IOHIDElementRef element = IOHIDValueGetElement(value);
  IOHIDElementCookie cookie = IOHIDElementGetCookie(element);
  IOHIDDeviceRef device_ref = IOHIDElementGetDevice(element);

  // Find device. TODO: Pass device to avoid this.
  HidDevice* hid_device = nullptr;
  for (HidDevice& device : devices_) {
    if (device.device_ref == device_ref) {
      hid_device = &device;
      break;
    }
  }
  if (hid_device == nullptr) {
    return;
  }

  const int int_value = IOHIDValueGetIntegerValue(value);

  // Find cookie for buttons. TODO: Provide better map to avoid lookup.
  for (std::size_t i = 0; i < hid_device->button_map.size(); ++i) {
    HidButtonInfo& button_info = hid_device->button_map[i];
    if (button_info.cookie == cookie) {
      HandleButtonEvent(&hid_device->device, button_info.button_id, int_value);
      return;
    }
  }

  // Find cookie for axes. TODO: Provide better map to avoid lookup.
  for (std::size_t i = 0; i < hid_device->axis_map.size(); ++i) {
    HidAxisInfo& axis_info = hid_device->axis_map[i];
    if (axis_info.cookie == cookie) {
      HandleAxisEvent(&hid_device->device, axis_info.axis_id, int_value,
          axis_info.minimum, axis_info.maximum, 0, 0);
      return;
    }
  }
}

}  // namespace gamepad

#endif  // __APPLE__
