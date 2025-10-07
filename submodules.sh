#!/bin/bash

git submodule update --init
(cd externals/dawn/dawn && git submodule update --init "third_party/abseil-cpp" "third_party/protobuf" "third_party/spirv-headers/src" "third_party/spirv-tools/src" "third_party/vulkan-headers/src" "third_party/vulkan-utility-libraries/src" "third_party/jinja2" "third_party/markupsafe")
(cd externals/wamr/zydis/dependencies && git submodule update --init)
