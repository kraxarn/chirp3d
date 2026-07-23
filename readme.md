# chirp3d

Modern, lightweight and cross-platform 3D-focused game engine.
Somewhat related to [chirp](https://github.com/kraxarn/chirp),
but with a stronger focus on modern 3D support over supporting every platform in existence.

# status

Currently very early in development and not very usable quite yet.

# features

* No AI slop, all bugs were proudly created by me.

# goals (non-final)

* The entire engine should be lightweight, and smaller than 10 MB.
* The engine should be directly native for each supported platform.

# why c?

* It's easier to interface with lower-level libraries.
* It compiles superfast.
* I like shooting myself in the foot :3

# platform support

* Linux (x86_64, aarch64) and Android (armeabi-v7a, arm64-v8a) through Vulkan 1.0.
* macOS (x86_64, arm64) and iOS through Metal, or Vulkan 1.0 using MoltenVK.
* Windows (x86, x86_64, arm64) through Direct3D 12 (FL 11_0, RBT 2), or Vulkan 1.0.

# compiler support

Any compiler with C23 support should work, including Clang 19+ (recommended) or GCC 15+.
MSVC is currently not supported, as it doesn't support C23 yet.

# why yet another game engine?

It's fun to make and very cool to have your own game engine B)
