#!/bin/bash

# update freetype from upstream
(cd freetype/freetype && git fetch --all && (git checkout -b master || git checkout master) && git reset --hard upstream/HEAD && git push)

echo
echo

function pbranch {
	echo
	pwd
	echo
	git checkout . # clear local changes
	git checkout -b master || git checkout master
	git branch --set-upstream-to=origin/HEAD master
	git fetch --all
	git reset --hard origin/HEAD
}
export -f pbranch
git submodule foreach pbranch

echo
echo

# restore mbedtls
git submodule update --init --recursive mbedtls/mbedtls

# restore openxr - newer versions are not supported by hardware
git submodule update openxr-sdk/OpenXR-SDK

# restore glslang - newer versions produce invalid spirv
git submodule update spirv/glslang
