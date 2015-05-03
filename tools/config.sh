SYSTEM_HEADER_PROJECTS="boot kernel"
PROJECTS="boot kernel"

export MAKE=${MAKE:-make}
export HOST=${HOST:-x86_64-elf}

export AR=${HOST}-ar
export AS=${HOST}-as
export CC=${HOST}-gcc
export CPP=${HOST}-cpp
export LD=${HOST}-ld
export OBJDUMP=${HOST}-objdump
export OBJCOPY=${HOST}-objcopy

export PREFIX=/usr
export EXEC_PREFIX=$PREFIX
export LIBDIR=$EXEC_PREFIX/lib
export INCLUDEDIR=$PREFIX/include

export CFLAGS='-O2 -g'
export CPPFLAGS='-DCONFIG_ARCH=1'

# Configure the cross-compiler to use the desired system root.
export CC="$CC --sysroot=$PWD/sysroot"

# Work around that the -elf gcc targets doesn't have a system include directory
# because configure received --without-headers rather than --with-sysroot.
if echo "$HOST" | grep -Eq -- '-elf($|-)'; then
  export CC="$CC -isystem=$INCLUDEDIR"
fi
