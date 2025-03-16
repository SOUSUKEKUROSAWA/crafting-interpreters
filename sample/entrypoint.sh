#!/bin/bash

echo "Building interpreter..."
cd /app/src

if [ ! -d "tool/.dart_tool" ]; then
  make get
else
  echo "Dependencies already installed. Skipping make get."
fi

if [ ! -d "build" ]; then
  make
else
  echo "Interpreter already built. Skipping make."
fi

if [ ! -f "clox" ]; then
  make clox
else
  echo "Clox already built. Skipping make clox."
fi

if [ ! -d "build/java" ]; then
  make jlox
else
  echo "Jlox already built. Skipping make jlox."
fi

exec bash
