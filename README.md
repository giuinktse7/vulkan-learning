Vulkan Learning
==================================================

This repository contains an old version of the [Vulkan Game Editor](https://github.com/giuinktse7/vulkan-game-editor).

**NOTE**: This repository contains low-level Vulkan code for handling Device, SwapChain, Surfaces, etc. These concepts are handled through the QT framework in [Vulkan Game Editor](https://github.com/giuinktse7/vulkan-game-editor), but it might be desirable to use a custom implementation in the future. If so, the code in this repository is a good starting place. `graphics/engine.cpp` is a good place to start.

# Compiling

## 1. Install libraries

```bash
vcpkg install liblzma protobuf nlohmann-json stb pugixml libarchive
```

# Terminology

## Action

An action is an event that can occur and can be undone/redone.
