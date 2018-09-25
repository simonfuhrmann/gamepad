#ifdef __linux__

#include "gamepad_linux.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstring>
#include <unistd.h>

#include <string>

namespace gamepad {
namespace {
void EvdevPrintEventBits(struct libevdev* dev, unsigned int type, unsigned int max) {
	for (unsigned int i = 0; i <= max; i++) {
		if (!libevdev_has_event_code(dev, type, i))
			continue;
		printf("    Event code %i - %s", i, libevdev_event_code_get_name(type, i));
		if (type == EV_ABS) {
      const struct input_absinfo* abs = libevdev_get_abs_info(dev, i);
      printf(" (value=%d min=%d max=%d fuzz=%d flat=%d res=%d)\n",
          abs->value, abs->minimum, abs->maximum,
          abs->fuzz, abs->flat, abs->resolution);
    } else {
      printf("\n");
    }
	}
}

void EvdevPrintEvents(struct libevdev* evdev) {
	printf("Attached: %s\n", libevdev_get_name(evdev));
	for (unsigned int i = 0; i <= EV_MAX; i++) {
		if (libevdev_has_event_type(evdev, i)) {
			printf("  Event type %d - %s\n", i, libevdev_event_type_get_name(i));
    }
		switch(i) {
			case EV_KEY:
				EvdevPrintEventBits(evdev, EV_KEY, KEY_MAX);
				break;
			case EV_REL:
				EvdevPrintEventBits(evdev, EV_REL, REL_MAX);
				break;
			case EV_ABS:
				EvdevPrintEventBits(evdev, EV_ABS, ABS_MAX);
				break;
			case EV_LED:
				EvdevPrintEventBits(evdev, EV_LED, LED_MAX);
				break;
		}
  }
}
}  // namespace

SystemImpl::~SystemImpl() {
  for (EvdevDevice& device : devices_) {
    EvdevCleanup(&device);
  }
}

void
SystemImpl::ProcessEvents() {
  if (!initialized_) {
    Initialize();
    initialized_ = true;
  }
  EvdevReadInputs();
}

void
SystemImpl::Initialize() {
  // TODO: Search in filesystem.
  EvdevInitialize("/dev/input/event5");
}

void
SystemImpl::EvdevCleanup(EvdevDevice* device) {
  if (device->evdev != nullptr) {
    libevdev_free(device->evdev);
    device->evdev = nullptr;
  }
  if (device->file_descriptor >= 0) {
    ::close(device->file_descriptor);
    device->file_descriptor = -1;
  }
}

void
SystemImpl::EvdevInitialize(const std::string& filename) {
  EvdevDevice device;
  device.filename = filename;

  device.file_descriptor = ::open(filename.c_str(), O_RDONLY|O_NONBLOCK);
  if (device.file_descriptor < 0) {
    fprintf(stderr, "Failed to open event file\n");
    return;
  }

  int rc = libevdev_new_from_fd(device.file_descriptor, &device.evdev);
  if (rc < 0 || device.evdev == nullptr) {
    fprintf(stderr, "Failed to init libevdev: %s\n", ::strerror(-rc));
    EvdevCleanup(&device);
    return;
  }

  device.device.vendor_id = libevdev_get_id_vendor(device.evdev);
  device.device.product_id = libevdev_get_id_product(device.evdev);
  device.device.description = libevdev_get_name(device.evdev);
  EvdevPrintEvents(device.evdev);

  // Scan gamepad buttons.
  device.key_map.resize(KEY_MAX);
  int num_buttons = 0;
  for (unsigned int i = 0; i <= KEY_MAX; i++) {
    if (libevdev_has_event_code(device.evdev, EV_KEY, i)) {
      device.key_map[i].button_id = num_buttons;
      num_buttons += 1;
    }
  }
  device.device.buttons.resize(num_buttons, false);

  // Scan gamepad axes.
  device.axis_map.resize(ABS_MAX);
  int num_axes = 0;
  for (unsigned int i = 0; i <= ABS_MAX; i++) {
    if (libevdev_has_event_code(device.evdev, EV_ABS, i)) {
      const struct input_absinfo* abs = libevdev_get_abs_info(device.evdev, i);
      device.axis_map[i].axis_id = num_axes;
      device.axis_map[i].minimum = abs->minimum;
      device.axis_map[i].maximum = abs->maximum;
      device.axis_map[i].flat = abs->flat;
      device.axis_map[i].fuzz = abs->fuzz;
      num_axes += 1;
    }
  }
  device.device.axes.resize(num_axes, 0.0f);

  // Register device and notify client.
  devices_.push_back(device);
  if (attached_handler_) {
    attached_handler_(&devices_.back().device);
  }
}

void
SystemImpl::EvdevReadInputs() {
  bool clean_up_devices = false;
  for (EvdevDevice& device : devices_) {
    struct input_event event;
    while (true) {
      int rc = libevdev_next_event(device.evdev, LIBEVDEV_READ_FLAG_NORMAL, &event);

      // If events have been dropped, sync up.
      if (rc == LIBEVDEV_READ_STATUS_SYNC) {
        while (rc == LIBEVDEV_READ_STATUS_SYNC) {
          EvdevProcessEvent(&device, event);
          rc = libevdev_next_event(device.evdev, LIBEVDEV_READ_FLAG_SYNC, &event);
        }
      }

      // Process successfully read events.
      if (rc == LIBEVDEV_READ_STATUS_SUCCESS) {
        EvdevProcessEvent(&device, event);
        continue;
      }

      // Break if no more events are pending.
      if (rc == -EAGAIN) break;

      // Other cases are errors. Remove device.
      EvdevCleanup(&device);
      clean_up_devices = true;
      break;
    }
  }

  // Detach devices that have been removed.
  if (clean_up_devices) {
    for (auto iter = devices_.begin(); iter != devices_.end();) {
      if (iter->evdev == nullptr) {
        if (detached_handler_) {
          detached_handler_(&iter->device);
        }
        iter = devices_.erase(iter);
      } else {
        iter++;
      }
    }
  }
}

void
SystemImpl::EvdevProcessEvent(EvdevDevice* device, const struct input_event& event) {
  // Not sure why this is necessary. Ignore.
  if (event.type == EV_SYN) {
    return;
  }

  if (event.type == EV_KEY) {
    // Handle button event.
    EvdevKeyInfo& key_info = device->key_map[event.code];
    HandleButtonEvent(&device->device, key_info.button_id, event.value);
  } else if (event.type == EV_ABS) {
    // Handle axis event.
    EvdevAxisInfo& axis_info = device->axis_map[event.code];
    HandleAxisEvent(&device->device, axis_info.axis_id, event.value,
        axis_info.minimum, axis_info.maximum, axis_info.fuzz, axis_info.flat);
  }
}

}  // namespace gamepad

#endif  // __linux__
