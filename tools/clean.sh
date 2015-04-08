#!/bin/sh
set -e
. ./tools/config.sh

for PROJECT in $PROJECTS; do
  $MAKE -C $PROJECT clean
done

rm -rfv sysroot
