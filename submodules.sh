#!/bin/bash

set -e

update_submodule()
{
	local repo_path="$1"
	local sub_path="$2"
	pushd "$repo_path" >/dev/null
	if [ -d "$sub_path/.git" ] || [ -f "$sub_path/.git" ]; then
		echo "skipping $repo_path/$sub_path"
	else
		echo "initializing $repo_path/$sub_path"
		git submodule update --init "$sub_path"
	fi
	popd >/dev/null
}

git submodule init

git config --file .gitmodules --get-regexp path | awk '{print $2}' | while read -r sub; do
	update_submodule "." "$sub"
done

update_submodule "externals/dawn/dawn" "third_party/abseil-cpp"
update_submodule "externals/dawn/dawn" "third_party/glslang/src"
update_submodule "externals/dawn/dawn" "third_party/jinja2"
update_submodule "externals/dawn/dawn" "third_party/markupsafe"
update_submodule "externals/dawn/dawn" "third_party/protobuf"
update_submodule "externals/dawn/dawn" "third_party/spirv-headers/src"
update_submodule "externals/dawn/dawn" "third_party/spirv-tools/src"
update_submodule "externals/dawn/dawn" "third_party/vulkan-headers/src"
update_submodule "externals/dawn/dawn" "third_party/vulkan-utility-libraries/src"
update_submodule "externals/dawn/dawn" "third_party/webgpu-headers/src"
#update_submodule "externals/mbedtls/mbedtls" "framework"
#update_submodule "externals/mbedtls/mbedtls" "tf-psa-crypto"
#update_submodule "externals/mbedtls/mbedtls/tf-psa-crypto" "framework"
update_submodule "externals/wamr/zydis" "dependencies/zycore"

git_ignore_file()
{
	local repo_path="$1"
	local sub_path="$2"
	pushd "$repo_path" >/dev/null
	git update-index --assume-unchanged "$sub_path"
	popd >/dev/null
}

git_ignore_file "externals/dawn/dawn" "CMakeLists.txt"
git_ignore_file "externals/dawn/dawn" "src/dawn/common/Constants.h"
git_ignore_file "externals/dawn/dawn" "third_party/CMakeLists.txt"
git_ignore_file "externals/quickhull/quickhull" "QuickHull.cpp"
