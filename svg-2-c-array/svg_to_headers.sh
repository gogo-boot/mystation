#!/bin/bash
# SVG to C-array header conversion for MyStation e-paper icons.
# Converts SVG files to PNGs via Inkscape, then to C header bitmap arrays.
#
# Usage: bash svg_to_headers.sh <size>
# Example: bash svg_to_headers.sh 64
#
# Output: ../lib/bitmap_images/<size>x<size>/*.h

set -e

if [ -z "$1" ]; then
  echo "Usage: bash svg_to_headers.sh <size>"
  echo "Example: bash svg_to_headers.sh 64"
  exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PYTHON="${PYTHON:-$SCRIPT_DIR/venv/bin/python3}"
SVG_FILES="$SCRIPT_DIR/svg/*.svg"
PNG_PATH="$SCRIPT_DIR/png/${1}x${1}"
HEADER_PATH="$SCRIPT_DIR/../lib/bitmap_images/${1}x${1}"

# Step 1: Convert SVG to PNG
mkdir -p "$PNG_PATH"
for f in $SVG_FILES; do
  echo "Converting .svg to .png: $(basename $f)..."
  out="$PNG_PATH/$(basename $f .svg).png"
  inkscape -w ${1} -h ${1} "$f" -o "$out" --export-background="#ffffff"
done

# Step 2: Convert PNG to C header files
rm -rf "$HEADER_PATH"
mkdir -p "$HEADER_PATH"
for f in "$PNG_PATH"/*.png; do
  header_name="$(basename $f .png | tr -s -c '[:alnum:]\n' '_')_${1}x${1}.h"
  out="$HEADER_PATH/$header_name"
  echo "Generating header: $header_name..."
  $PYTHON "$SCRIPT_DIR/png_to_header.py" -i "$f" -o "$out"
done

echo "Done (${1}x${1})."
