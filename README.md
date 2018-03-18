C++ 3D game engine designed for strategy games.
A long-term hobby project that turned out to be quite BIG.

# Components

 - Core Library
   - Operating system abstraction
     - memory management
     - threading & synchronization
     - filesystem
     - networking (via Enet)
   - Math
     - glsl like: real, vec3, quat, mat4, degs, rads, ...
     - no templates
   - Geometry
     - shapes: line (ray, segment), triangle, plane, aabb, sphere, ...
     - intersections & collisions
   - Entities
     - this is the heart of all higher-level APIs
     - entities are identified by pointer and, optionally, by a number
     - entities may have any number of predefined components
       - single entity may not have multiple instances of the same component
       - components are treated as byte-buffers - no destructors etc.
     - entities may be aggregated into groups for easier processing
     - easy serialization
   - Assets
     - thread-aware loading
     - transparent on-demand hot-reloading
     - automatic dependencies

 - Client Library
   - Low-level API
     - Window management & events (via GLFW)
     - Graphics
       - currently OpenGL 4.4
         - the plan is to switch to Vulkan only (when there is enough time)
       - actual objective abstraction of most gl objects
         - semi-debugging layer that validates that the objects are correctly bound
     - Sound (via soundio)
     - Gui
       - entities API only
       - flexible automatic layouting
       - flexible skinning
   - Engine (high-level) API
     - the scene is defined entirely by entities
       - cameras, renderables, lights & shadows, listeners, voices, ...
     - pipeline-like processing using threads dedicated to specific tasks
     - *30 000 objects* at 30 fps (cpu-bound)
     - all rendering is through automatic instancing
     - automatic shadow maps

 - Tools
   - asset-processor
     - texture loading (via DevIL)
       - DevIL has lgpl license, which may cause trouble, and is planned to be replaced
     - meshes loading (via Assimp)
     - sound (via Ogg/Vorbis and dr_libs)
     - shader
       - custom $preprocessor with support for includes, conditional composition, token replacements etc...
     - font (via freetype)
       - currently generates bitmaps
       - the goal is to have flexible distance-field textures, which is a work-in-progress
   - asset-database
     - manages automatic asset processing
     - only processes assets that are out-of-date
     - can be set to listen to changes and act immediately
       - notifies all running games when an asset has changed so that the engine reloads it
     - assets are configured in simple ini-like files (good for version-control-systems)
     - easily extendable to any custom files
   - asset-generator
     - given a path to a folder, it will analyze all files in it and generate basic asset configuration

 - notes
   - Cage is NOT an IDE or editor
   - when developing with Cage, you use your preferred IDE or editor
   - all interaction with Cage is with c++ code, ini configuration files and the tools

# Features

## Ease of use
 - as surprising as it is, Cage's first design goal is to be super easy to use
 - ease of use here refers to single, easy to understand, programming API
 - even that the engine is composed from several libraries, they are all hidden behind the scene
 - the API consists of both functions and classes, whichever is more convenient

## Portable
 - runs on Windows and Linux
 - (MacOS does not have required OpenGL version, but is otherwise supported)

## Friendly Licensing
 - MIT license is one of the shortest licenses possible
 - commercial or not, closed source or open source, ... whatever :D

# Examples
 - https://github.com/ucpu/grid
