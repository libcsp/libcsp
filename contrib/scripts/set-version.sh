#!/usr/bin/bash

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <new-version>"
    exit 1
fi

NEW_VERSION=$1

# meson.build
sed -i "s/version: '[^']*'/version: '$NEW_VERSION'/" meson.build

# wscript
sed -i "s/VERSION = '[^']*'/VERSION = '$NEW_VERSION'/" wscript

# CMakeLists.txt
sed -i "s/project(CSP VERSION [^)]*)/project(CSP VERSION $NEW_VERSION)/" CMakeLists.txt

# Check
git diff
