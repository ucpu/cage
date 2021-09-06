Cage is game engine/framework designed for programmers with focus on dynamic games and procedural content generation.

It started as a hobby/learning project, but has become fairly feature rich and mature over the years.

# Features

## Designed for programmers

- Cage is a collection of libraries and tools, not yet another editor.
- No visual programming - use c++ for the engine and for the game.
- All tools and assets are configured with simple ini-like text files, which is suitable for version control systems.

## Ease of use

- Cage is programming framework that creates a consistent basis for engine, tools and game development.
- It encourages readable, maintainable and safe code while also allowing for quick prototyping.
- Performance optimizations are added as necessary, preferably without changing api.

## Portable

- Cage runs on Windows and Linux.
- It is mostly self-contained.
  - Most dependencies are accessed as git submodules and compiled in.
  - There are just few packages required on linux, none on windows.
- The Core library has no dependencies on graphics or sound and is suitable for running on headless servers.

## Friendly licensing

- MIT license lets you use Cage for personal and commercial products alike, for free, forever.
- All 3rd-party licenses are copied to single redistributable directory during build configuration.

# Examples

## Degrid

![Degrid](https://raw.githubusercontent.com/ucpu/degrid/master/screenshots/3.png)
[Degrid](https://github.com/ucpu/degrid) - simple but mostly complete game with birds-eye-view of a spaceship fighting against hundreds of geometrical enemies.

## Cragsman

![Cragsman](https://raw.githubusercontent.com/ucpu/cragsman/master/screenshots/2.png)
[Cragsman](https://github.com/ucpu/cragsman) - a game demo with procedurally generated terrain and simple physics

## Space Ants

![Space-ants](https://raw.githubusercontent.com/ucpu/space-ants/master/screenshots/2.png)
[Space-ants](https://github.com/ucpu/space-ants) - visual simulation of thousands little spaceships at war

## Cage Examples

![Cage-examples](https://raw.githubusercontent.com/ucpu/cage-examples/master/screenshots/3.png)
[Cage-examples](https://github.com/ucpu/cage-examples) - a collection of test scenes and applications

# Contents

## Core library

- Framework stuff (strings, logging, configuration, events)
- Operating system abstraction (memory, filesystems, threading, networks)
- GLSL-like math and geometry primitives
- Image, triangle meshes and sounds en/decoding and algorithms
- Entity + components framework
- Assets management
- Extensive tests

## Engine library

- Provides low-level/generic engine functionality
- Scene description with entities
- Window and input management
- Gameloop and timing
- Objective layer on top of OpenGL
  - Long term plan is to switch to Vulkan only
- Sound processing
- Gui
- Graphics effects:
  - hdr, bloom, tonemapping, gamma correction
  - depth of field
  - motion blur (per object)
  - ssao
  - fxaa

## Simple library

- Provides simpler interface for typical games
- Roughness/metallic workflow
- Pipeline-like processing using multiple threads
  - *50 000 objects* at 30 fps (cpu-bound)
  - All objects goes through automatic instancing
- Automatic shadowmaps

## Tools

- Asset processor - converts assets from wide variety of interchange formats to Cage specific formats
- Asset database - manages asset configuration and automatic processing
- Asset analyze - generates initial asset configuration for most files
- Image resize - simple batch resizing of images
- Image convert - simple batch image format conversion
- Image channels - split or join multiple channels to/from single image
- Image atlas - slices a single image atlas grid into a sequence of individual images

# Building

- See [BUILDING](BUILDING.md).

# Contributing

- Contributions are welcome!
- Questions, bug reports and feature requests are best done on github issues tracker and pull requests.
