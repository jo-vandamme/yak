#!/bin/sh
set -e
. ./tools/headers.sh

for PROJECT in $PROJECTS; do
  DESTDIR="$PWD/sysroot" $MAKE -C $PROJECT install
done

. ./tools/update_image.sh
