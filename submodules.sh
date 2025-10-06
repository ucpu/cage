#!/bin/bash

git submodule update --init
(cd externals/dawn/dawn/third_party && git submodule update --init)
(cd externals/wamr/zydis/dependencies && git submodule update --init)
