#!/bin/bash

# update freetype from upstream
(cd freetype/freetype && git fetch upstream HEAD && git reset --hard upstream/master && git push)


function pbranch {
	git checkout -b master
	git checkout master
	git branch --set-upstream-to=origin/HEAD master
	git pull --ff-only
	git submodule update --init --recursive
}
export -f pbranch
git submodule foreach pbranch


# restore openxr - newer versions are not supported by hardware
git submodule update openxr-sdk/OpenXR-SDK

# restore wamr & zydis
git submodule update --init --recursive wamr/wamr
git submodule update --init --recursive wamr/zydis
