# Gamepad -- A simple interface for gamepads and joysticks

Gamepad is a simple library to read inputs from your gamepad or joystick
controllers, such as the XBox One or the PS4 controller. The currently
supported platforms are

* Linux, using the modern event interfaces (`/dev/input/event*`)
* MacOS, using the extremely horrible IOKit framework

In particular, the library allows to listen to device attach/detach events,
button down/up events, and axis move events, using a convenient callback
interface. The library does not attempt to map specific buttons or axes to
a common interface. Button and axis order is presented as reported by the
device.

Example code (see `main.cc` for details):

````c++
int main() {
  std::unique_ptr<gamepad::System> gamepad = gamepad::System::Create();
  gamepad->RegisterAttachHandler(device_attached);
  gamepad->RegisterDetachHandler(device_detached);
  gamepad->RegisterButtonUpHandler(button_event);
  gamepad->RegisterButtonDownHandler(button_event);
  gamepad->RegisterAxisMoveHandler(axis_event);

  while (true) {
    gamepad->ProcessEvents();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  return 0;
}
````

The library only supports joystick-like devices. Mouse and keyboard are not
supported.

## Linux support

On Linux, events are read from the event character devices. For ease of
development, the `libevdev` library is used. Certain features of the gamepad
are supported if reported by the gamepad, such as

* Dead-zone (flat value): Tiny values are reported as zero to reduce noise
* Filtering (fuzz value): Tiny changes are not reported to reduce noise

Only joystick-like interafaces are scanned.

## MacOS X support

On MacOS X, events are managed using the IOKit framework and by filtering
all avaliable HID devices by `GD_GamePad`, `GD_Joystick` and
`GD_MultiAxisController`. Mouse and keyboard are not supported.

Note that MacOS does not support the XBox One controller by default (additional
drivers are necessary). The PS4 controller reports an insane amount of axis
events on MacOS, unlike on Linux. The reason is unknown. These axes should be
ignored.
