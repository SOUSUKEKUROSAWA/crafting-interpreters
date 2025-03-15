#!/bin/bash

echo "Building interpreter..."
cd /app/src

if [ ! -d "tool/.dart_tool" ]; then
  make get
fi

if [ ! -d "build" ]; then
  make
fi

if [ ! -f "clox" ]; then
  make clox
fi

if [ ! -d "build/java" ]; then
  make jlox
fi

exec bash
