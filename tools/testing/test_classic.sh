#!/bin/sh
set -euf

testers-cmake --verbose --run-clang-format-script $ROOT_DIR/tools/testing/run-clang-format.py
