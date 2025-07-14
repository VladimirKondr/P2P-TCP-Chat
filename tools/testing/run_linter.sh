#!/bin/sh
set -eu

# Run from task directory or pass the task path as an argument

SCRIPT_DIR=$(dirname $0)
TASK_DIR=${1-$PWD}
BUILD_DIR=$SCRIPT_DIR/../../build

$SCRIPT_DIR/run-clang-format.py --clang-format-executable clang-format -r $TASK_DIR
echo "Clang-format: OK"


if [[ $(find $TASK_DIR -name "BUILD") == *"BUILD" ]]; then
    if command -v bazel &>/dev/null; then
        bazel run @hedron_compile_commands//:refresh_all || true
    fi
    find $TASK_DIR -name '*.cpp' | xargs clang-tidy -p $BUILD_DIR/..
else
    find $TASK_DIR -name '*.cpp' | xargs clang-tidy -p $BUILD_DIR
fi

echo "Clang-tidy: OK"
