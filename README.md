# libudev - Termux Compatible Implementation

This is a Termux/Android compatible implementation of libudev that provides the same API as the official systemd libudev library.

## Overview

This library provides a mock implementation of libudev specifically designed for Termux/Android environments where the standard systemd udev is not available. It implements the essential libudev API functions needed by applications like KWin.

## Features

- **API Compatible**: Provides the same function signatures as official libudev
- **Mock Device Support**: Creates mock devices for DRM/GPU detection
- **Minimal Dependencies**: Only requires standard C library and pthread
- **Meson Build System**: Uses the same build system as official systemd/libudev

## Architecture

The library consists of several modules:

- `libudev.c` - Main library context management
- `libudev-device.c` - Device representation and properties
- `libudev-enumerate.c` - Device enumeration and filtering
- `libudev-list.c` - List operations for properties and entries
- `libudev-monitor.c` - Device event monitoring (mock implementation)
- `libudev-queue.c` - Event queue management
- `libudev-hwdb.c` - Hardware database interface
- `libudev-util.c` - Utility functions

## Mock Behavior

### Device Enumeration
- Creates mock DRM devices (`/dev/dri/card0`, `/dev/dri/card1`) when filtering for `drm` subsystem
- Supports `card[0-9]` sysname pattern matching
- Returns appropriate device properties for GPU detection

### Device Properties
- Mock devices include standard properties like `ID_SEAT`, `DEVTYPE`, `MAJOR`
- Supports common sysattr queries like `boot_vga`
- Device nodes point to standard DRM device paths

### Event Monitoring
- Monitor functions are implemented but return no events (suitable for static device scenarios)
- Socket creation works but no actual netlink communication occurs

## Building

```bash
meson setup build
meson compile -C build
meson install -C build
```

## Installation

After building, the library will be installed as:
- Library: `libudev.so.1`
- Headers: `libudev.h`
- pkg-config: `libudev.pc`

## Usage with KWin

This library is specifically designed to work with KWin's libinput backend on Termux. KWin will use this library for:

1. **GPU Detection**: Enumerating DRM devices for rendering
2. **Device Properties**: Querying device capabilities and seat assignments
3. **Input Device Management**: Though actual input handling is done via termux-display-client

## Compatibility

This implementation focuses on the subset of libudev functionality actually used by KWin and similar applications. It may not be suitable for applications that require:

- Real hardware device enumeration
- Actual udev event monitoring
- Complete sysfs attribute access
- Hardware database lookups

## License

LGPL-2.1-or-later (same as official systemd/libudev)

## Integration

To use this library with KWin on Termux:

1. Build and install this libudev implementation
2. Build KWin with the modified libinput backend
3. Ensure termux-display-client is available for actual input/display functionality

The library provides the udev interface that KWin expects while the actual hardware interaction is handled by the Termux-specific components.