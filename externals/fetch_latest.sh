#!/bin/bash

# update freetype from upstream
(cd freetype/freetype && git fetch upstream HEAD && git reset --hard upstream/master)


function pbranch {
	git checkout -b master
	git checkout master
	git branch --set-upstream-to=origin/HEAD master
	git pull
	git submodule update --init --recursive
}
export -f pbranch
git submodule foreach pbranch


# restore protobuf - newer versions are incompatible with the protoc compiler
git submodule update protobuf/protobuf

# restore openxr - newer versions are not supported by hardware
git submodule update openxr-sdk/OpenXR-SDK
