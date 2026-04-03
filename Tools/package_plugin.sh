#!/bin/bash
#
# Package DecalBaker plugin for distribution.
# Builds the plugin and creates a ready-to-install zip file.
#
# Usage:
#   ./Tools/package_plugin.sh [UE_ROOT]
#
# Examples:
#   ./Tools/package_plugin.sh "/Users/Shared/Epic Games/UE_5.7"
#   ./Tools/package_plugin.sh "C:\Program Files\Epic Games\UE_5.7"
#
# Output:
#   Dist/DecalBaker-v0.1.0.zip  (ready to distribute)
#

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
PLUGIN_DIR="$REPO_ROOT/DecalBaker"
DIST_DIR="$REPO_ROOT/Dist"
VERSION="0.1.0"
OUTPUT_NAME="DecalBaker-v${VERSION}"

# --- Find UE ---
UE_ROOT="${1:-}"

if [ -z "$UE_ROOT" ]; then
    # Auto-detect on macOS
    if [ -d "/Users/Shared/Epic Games/UE_5.7" ]; then
        UE_ROOT="/Users/Shared/Epic Games/UE_5.7"
    elif [ -d "/Users/Shared/Epic Games/UE_5.6" ]; then
        UE_ROOT="/Users/Shared/Epic Games/UE_5.6"
    # Auto-detect on Windows (Git Bash / MSYS)
    elif [ -d "C:/Program Files/Epic Games/UE_5.7" ]; then
        UE_ROOT="C:/Program Files/Epic Games/UE_5.7"
    else
        echo "ERROR: Cannot find Unreal Engine installation."
        echo "Usage: $0 <UE_ROOT>"
        echo "  e.g. $0 \"/Users/Shared/Epic Games/UE_5.7\""
        exit 1
    fi
fi

echo "============================================"
echo "  DecalBaker Plugin Packager"
echo "============================================"
echo "  UE Root:  $UE_ROOT"
echo "  Plugin:   $PLUGIN_DIR"
echo "  Output:   $DIST_DIR/$OUTPUT_NAME.zip"
echo "============================================"

# --- Find RunUAT ---
if [ "$(uname)" = "Darwin" ]; then
    RUN_UAT="$UE_ROOT/Engine/Build/BatchFiles/RunUAT.sh"
else
    RUN_UAT="$UE_ROOT/Engine/Build/BatchFiles/RunUAT.bat"
fi

if [ ! -f "$RUN_UAT" ]; then
    echo "ERROR: RunUAT not found at $RUN_UAT"
    exit 1
fi

# --- Clean previous build ---
STAGING_DIR="$DIST_DIR/Staging/$OUTPUT_NAME"
rm -rf "$DIST_DIR/Staging"
rm -f "$DIST_DIR/$OUTPUT_NAME.zip"
mkdir -p "$STAGING_DIR"

# --- Build Plugin ---
echo ""
echo "Building plugin..."
echo ""

"$RUN_UAT" BuildPlugin \
    -Plugin="$PLUGIN_DIR/DecalBaker.uplugin" \
    -Package="$STAGING_DIR/DecalBaker" \
    -Rocket \
    -TargetPlatforms=Mac \
    2>&1 | tail -20

if [ $? -ne 0 ]; then
    echo "ERROR: Plugin build failed"
    exit 1
fi

# --- Add install script ---
cp "$REPO_ROOT/Tools/install_plugin.py" "$STAGING_DIR/"
cp "$REPO_ROOT/Tools/INSTALL.txt" "$STAGING_DIR/" 2>/dev/null || true

# --- Create zip ---
echo ""
echo "Creating distribution zip..."
cd "$DIST_DIR/Staging"
zip -r "$DIST_DIR/$OUTPUT_NAME.zip" "$OUTPUT_NAME" -x "*.DS_Store"

# --- Cleanup ---
rm -rf "$DIST_DIR/Staging"

echo ""
echo "============================================"
echo "  Package complete!"
echo "  $DIST_DIR/$OUTPUT_NAME.zip"
echo "============================================"
echo ""
echo "Distribute this zip to designers."
echo "They run: python install_plugin.py"
echo "Or just copy DecalBaker/ to their project's Plugins/ folder."
