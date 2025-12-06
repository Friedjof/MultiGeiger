#!/usr/bin/env bash

set -euo pipefail

SRC="src/config/config.default.hpp"
DST="src/config/config.hpp"

if [ ! -f "$SRC" ]; then
  echo "Missing default config at $SRC"
  exit 1
fi

if [ -f "$DST" ]; then
  echo "Using existing $DST"
else
  cp "$SRC" "$DST"
  echo "Created $DST from default"
fi
