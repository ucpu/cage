
# Git Submodules

First of all, make sure that git submodules were loaded too.
```bash
git pull
git submodule update --init --recursive
```

# Building

Cage build configuration is defined in [CMake](https://cmake.org/).
It can be used with Graphical User Interface, if available, or from command line.

## CMake Gui

Set "Where is the source code" to this directory.

Set "Where to build the directories" to some different directory, eg. "build".

Click "Configure", a dialog will appear that allows you to choose build tool.

Then click "Generate", "Open Project" and build the application as you normally would.

## CMake on Command Line

Navigate to the folder with the sources and prepare build directory.
```bash
mkdir build ; cd build
```

Choose a generator from those available for your platform.
```bash
cmake --help
```

Some generators are multi-configuration (eg. Visual Studio, XCode) and some are single-configuration (eg. make).

### Multi-configuration

Configure the project.
```bash
cmake -G"chosen generator name" ..
```

You can now open your build tool project and continue with it as you are used to, or you can build it from the command line.
```bash
cmake --build . --config RELWITHDEBINFO
```

### Single-configuration

Configure the project.
```bash
cmake -G"chosen generator name" -DCMAKE_BUILD_TYPE=RELWITHDEBINFO ..
```

You can now open your build tool project and continue with it as you are used to, or you can build it from the command line.
```bash
cmake --build .
```
