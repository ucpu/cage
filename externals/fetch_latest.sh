#!/bin/bash

function pbranch {
	git checkout -b master
	git checkout master
	git branch --set-upstream-to=origin/HEAD master
	git pull
	git submodule update --init --recursive
}

export -f pbranch

git submodule foreach pbranch
