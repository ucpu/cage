
# Git submodules

Make sure that git submodules are loaded too.
```bash
git pull
git submodule sync
git submodule update --init
```

# Building

## Windows

These instructions assume using bash that comes with "git for windows", but is not mandatory to the build itself.

Prepare build directory.
```bash
mkdir build ; cd build
```

Configure the project.
```bash
cmake ..
```

Open VS solution or build everything.
```bash
cmake --build . --config RELWITHDEBINFO
```

## Linux

These instructions apply to debian-based distributions (eg. ubuntu).
It may work on other distributions with little changes.

Install required packages.
```bash
sudo apt install xorg-dev libwayland-dev libxkbcommon-dev libpulse-dev libasound2-dev nasm libssl-dev
```

Prepare build directory.
```bash
mkdir build ; cd build
```

Configure the project.
```bash
cmake -DCMAKE_BUILD_TYPE=RELWITHDEBINFO ..
```

Build.
```bash
cmake --build . -- -j8 # 8 is number of processor cores
```

# Converting Assets

Run `cage-asset-database`. You may find it in build/result/relwithdebinfo.

