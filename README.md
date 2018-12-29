C++ 3D game engine designed primarily for strategy games.
A long-term hobby project that turned out to be quite BIG.

# Examples

## Grid
![Grid](https://raw.githubusercontent.com/ucpu/grid/master/screenshots/3.png)
[Grid](https://github.com/ucpu/grid) - simple but complete game

## Cragsman
![Grid](https://raw.githubusercontent.com/ucpu/cragsman/master/screenshots/2.png)
[Cragsman](https://github.com/ucpu/cragsman) - a game demo with procedurally generated terrain and simple physics

## Space Ants
![Grid](https://raw.githubusercontent.com/ucpu/space-ants/master/screenshots/2.png)
[Space-ants](https://github.com/ucpu/space-ants) - visual simulation of thousands little spaceships at war

## Cage Examples
![Grid](https://raw.githubusercontent.com/ucpu/cage-examples/master/screenshots/3.png)
[Cage-examples](https://github.com/ucpu/cage-examples) - a collection of test scenes and applications

# Features

## Ease of use
 - ease of use here refers to single, easy to understand programming API
 - even that the engine depends on several libraries, they are all hidden behind the scene
 - the API consists of both functions and classes, whichever is more convenient
 - the API is not polluted by any unnecessary includes (not even the standard libraries)

## Portable
 - runs on Windows and Linux
   - MacOS does not have required OpenGL version, but is otherwise supported
 - self-contained (as much as possible)
   - all dependencies are accessed as git submodules and compiled in
 - the Core library has no dependencies on graphics or sound and is therefore suitable to run on a server

## Friendly Licensing
 - MIT license is short and easy to understand
 - commercial or not, closed source or open source, ... whatever :D
 - (be sure to also check out the licenses for all dependencies)

## Notes
 - Cage is NOT an IDE or editor
   - when developing with Cage, you use your preferred editor
   - all interaction with Cage is in c++ code, ini configuration files and the CLI tools

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
   - Entities
     - this is the heart of all Cage's higher-level APIs
     - entities are identified by pointer and, optionally, by a number
     - entities may be aggregated into groups for easier processing
     - entities may have any number of predefined components
       - single entity may not have multiple instances of the same component
     - simple serialization
       - components are treated as byte-buffers (no destructors etc.)
   - Assets
     - thread-aware loading
     - transparent on-demand hot-reloading
     - automatic dependencies
   - Utilities

## Client Library
   - Low-level API
     - Window management & events (via GLFW)
     - Graphics
       - currently OpenGL 4.4
         - the plan is to switch to Vulkan only (when there is enough time)
       - actual objective abstraction of most OpenGL objects
         - semi-debugging layer that validates that the objects are correctly bound
     - Sound (via soundio)
       - support for spatial sound (multiple channels)
     - Gui
       - novel API through entities
       - flexible automatic layouting
       - flexible skinning
       - no custom (application defined) widgets, which allows fairly simple and efficient implementation
   - Engine (high-level) API
     - the scene is defined entirely by entities
       - cameras, renderables, lights & shadows, listeners, voices, ...
     - pipeline-like processing using threads dedicated to specific tasks
       - *50 000 objects* at 30 fps (cpu-bound)
       - all rendering is through automatic instancing to reduce draw call overhead
     - directional, spot and point lights
        - automatic shadow maps
     - roughness/metallic workflow (however the shaders could be improved a lot)

## Tools
   - asset-processor
     - texture loading (via DevIL)
       - DevIL has lgpl license, which may cause trouble, and is planned to be replaced
     - mesh loading (via Assimp)
     - sound (via Ogg/Vorbis and dr_libs)
     - shader
       - custom $preprocessor with support for includes, conditional composition, token replacements etc...
     - font (via freetype and msdfgen)
       - multi-distance-field scalable font textures
   - asset-database
     - manages automatic asset processing
     - only processes assets that are out-of-date
     - can be set to listen to changes and act immediately
       - notifies all connected games whenever an asset has changed so that the engine can reload it
     - assets are configured in simple ini-like files (good for version-control-systems)
     - easily extensible to any custom formats
   - asset-analyze
     - given a path to a folder, it will analyze all files in it and generate basic asset configuration

# Building

 - see [BUILDING](BUILDING.md).
