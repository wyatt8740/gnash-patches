#! /bin/sh
export LDFLAGS='-lGLU'
export LIBS='-lGLU'
export CXXFLAGS='--std=c++14'
./configure \
  --prefix=/usr \
  --libdir=/usr/lib/x86_64-linux-gnu \
  --sysconfdir=/etc \
  --enable-gui=gtk,dump,sdl \
  --enable-sound=sdl \
  --disable-npapi \
  --disable-kparts3 \
  --disable-kparts4 \
  --disable-plugins \
  --disable-docbook \
  --disable-ghelp \
  --enable-media=ffmpeg \
  --disable-testsuite \
  --enable-device=all \
  --without-gconf \
  --enable-renderer=all