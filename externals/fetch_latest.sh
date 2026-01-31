#!/bin/bash

# update freetype from upstream
(cd freetype/freetype && git fetch upstream HEAD && (git checkout -b master || git checkout master) && git reset --hard upstream/master && git push)

echo
echo

function pbranch {
	echo
	pwd
	echo
	git checkout . # clear local changes
	git checkout -b master || git checkout master
	git branch --set-upstream-to=origin/HEAD master
	git pull --ff-only
}
export -f pbranch
git submodule foreach pbranch

echo
echo

# restore openxr - newer versions are not supported by hardware
git submodule update openxr-sdk/OpenXR-SDK

# restore wamr & zydis
git submodule update --init --recursive wamr/wamr
git submodule update --init --recursive wamr/zydis
