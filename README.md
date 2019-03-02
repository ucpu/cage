C++ 3D game engine designed primarily for strategy games.
A long-term hobby project that turned out to be quite BIG.

# Examples

## Degrid

![Degrid](https://raw.githubusercontent.com/ucpu/degrid/master/screenshots/3.png)
[Degrid](https://github.com/ucpu/degrid) - simple but complete game

## Cragsman

![Cragsman](https://raw.githubusercontent.com/ucpu/cragsman/master/screenshots/2.png)
[Cragsman](https://github.com/ucpu/cragsman) - a game demo with procedurally generated terrain and simple physics

## Space Ants

![Space-ants](https://raw.githubusercontent.com/ucpu/space-ants/master/screenshots/2.png)
[Space-ants](https://github.com/ucpu/space-ants) - visual simulation of thousands little spaceships at war

## Cage Examples

![Cage-examples](https://raw.githubusercontent.com/ucpu/cage-examples/master/screenshots/3.png)
[Cage-examples](https://github.com/ucpu/cage-examples) - a collection of test scenes and applications

# Features

## Ease of Use

- ease of use here refers to single, easy to understand API
- even that the engine depends on several libraries, they are all hidden behind the scene
- the API consists of both functions and classes, whichever is more convenient
- the API is not polluted by any unnecessary includes (not even the standard libraries)

## Portable

- runs on Windows and Linux
  - MacOS does not have required OpenGL version, but is otherwise supported
- self-contained
  - all dependencies are accessed as git submodules and compiled in
  - (as much as possible - there are few package requirements on linux)
- the Core library has no dependencies on graphics or sound and is therefore suitable to run head-less

## Friendly Licensing

- MIT license is short and easy to understand
- commercial or not, closed source or open source, ... whatever :D
- (be sure to also check out the licenses for all dependencies)

## Notes

- Cage is NOT an IDE or editor
  - when developing with Cage, you use your preferred editor
  - all interaction with Cage is with c++ code, INI configuration files and CLI tools

# Components

## Core Library

- Operating system abstraction
  - memory management
  - threads & synchronization
  - filesystem
  - networking
- Math & geometry
  - glsl like: real, vec3, quat, mat4, degs, rads, ...
  - shapes: line (ray, segment), triangle, plane, aabb, sphere, ...
  - no templates, all is float
  - intersections & collisions
  - spatial acceleration structure (BVH)
- Entities
  - this is the heart of all Cage's higher-level APIs
  - entities are identified by pointer and, optionally, by a number
  - entities may be aggregated into groups for easier processing
  - entities may have any number of components
    - single entity may not have multiple instances of the same component
  - simple serialization
    - components are treated as byte-buffers (no destructors etc.)
- Assets
  - thread-aware loading
  - transparent on-demand hot-reloading
  - automatic dependencies
- Utilities

## Client Library

### Low-level API

- window management & events (via GLFW)
- graphics
  - currently OpenGL 4.4
    - the plan is to switch to Vulkan only (when there is enough time)
  - actual objective abstraction of most OpenGL objects
    - semi-debugging layer that validates that the objects are correctly bound
- sound (via soundio)
  - support for spatial sound (multiple channels)
- gui
  - API by entities
  - flexible automatic layouting
  - flexible skinning
  - no custom (application defined) widgets, this allows fairly efficient implementation

### Engine (high-level API)

- the scene is defined entirely by entities
  - transform (vec3 position, quaternion orientation, uniform scale)
  - cameras, renderables, lights
  - listeners, voices
- pipeline-like processing using multiple threads
  - *50 000 objects* at 30 fps (cpu-bound)
  - all rendering is through automatic instancing to reduce draw call overhead
- graphics effects
  - hdr, tonemapping, gamma correction
  - motion blur (per object)
  - ssao
  - fxaa
  - bloom
  - normal mapping
- directional, spot and point lights
  - automatic shadow maps
- roughness/metallic workflow
- stereoscopic rendering

## Tools

### Asset processor

- texture loading (via DevIL)
  - DevIL is planned to be replaced due to license issue (when suitable alternative is found)
- mesh loading (via Assimp)
- sound (via Ogg/Vorbis and dr_libs)
- shader
  - custom $preprocessor with support for includes, conditional composition, token replacements etc...
- font (via freetype and msdfgen)
  - multi-distance-field scalable font textures

### Asset database

- manages automatic asset processing
- only processes assets that are out-of-date
- can be set to listen to changes and act immediately
  - notifies all connected games whenever an asset has changed so that the engine can reload it
- assets are configured in simple ini-like files (good for version-control-systems)
- easily extensible to any custom formats

# Building

- see [BUILDING](BUILDING.md).
