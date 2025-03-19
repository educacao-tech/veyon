#!/usr/bin/env bash

set -e

export CMAKE_FLAGS="$CMAKE_FLAGS -DCPACK_DIST=rhel.9"

$1/.ci/common/linux-build.sh $@
$1/.ci/common/finalize-rpm.sh $1 $2
