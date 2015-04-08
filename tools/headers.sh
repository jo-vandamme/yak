#!/bin/sh
set -e
. ./tools/config.sh

mkdir -p sysroot

for PROJECT in $SYSTEM_HEADER_PROJECTS; do
  DESTDIR="$PWD/sysroot" $MAKE -C $PROJECT install-headers
done
