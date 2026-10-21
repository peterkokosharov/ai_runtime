#!/bin/bash
# LiteLLM TDE Applet - Quick Install Script
# Run: chmod +x install.sh && ./install.sh

set -e

echo "=== LiteLLM TDE Applet Installer ==="
echo ""

# Check if running as root (not recommended for build)
if [ "$EUID" -eq 0 ]; then
    echo "WARNING: Don't run as root. Run as normal user (sudo will be asked when needed)."
    exit 1
fi

# 1. Install system dependencies
echo "[1/5] Installing build dependencies..."
sudo apt update
sudo apt install -y \
    cmake \
    tdelibs14-trinity-dev \
    tdebase-trinity-dev \
    libtqt3-mt-dev \
    tqt3-dev-tools

# 2. Install LiteLLM
echo "[2/5] Installing LiteLLM..."
if command -v pipx &> /dev/null; then
    pipx install litellm
elif command -v pip3 &> /dev/null; then
    pip3 install --user litellm
else
    echo "ERROR: Neither pipx nor pip3 found. Install pipx first: sudo apt install pipx"
    exit 1
fi

# 3. Build applet
echo "[3/5] Building applet..."
cd "$(dirname "$0")"
mkdir -p build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/opt/trinity
make -j$(nproc)

# 4. Install applet
echo "[4/5] Installing applet to /opt/trinity..."
sudo make install

# 5. Restart panel
echo "[5/5] Restarting kicker panel..."
dcop kicker Panel restart

echo ""
echo "=== Done! ==="
echo "Right-click panel -> Add -> Applet -> LiteLLM Proxy"
echo "Then right-click applet -> Configure -> Select your config.yaml"