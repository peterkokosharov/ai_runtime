#!/bin/bash
# Install LiteLLM systemd user service
# Run: ./install-service.sh

set -e

SERVICE_FILE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/litellm.service"
USER_SYSTEMD_DIR="$HOME/.config/systemd/user"

echo "=== Installing LiteLLM systemd user service ==="
echo "Service file: $SERVICE_FILE"
echo "Target dir: $USER_SYSTEMD_DIR"

# Check if service file exists
if [ ! -f "$SERVICE_FILE" ]; then
    echo "Error: $SERVICE_FILE not found"
    exit 1
fi

# Create user systemd directory
mkdir -p "$USER_SYSTEMD_DIR"

# Copy service file
cp "$SERVICE_FILE" "$USER_SYSTEMD_DIR/"
echo "Copied service file"

# Reload systemd
systemctl --user daemon-reload
echo "Reloaded systemd"

# Enable and start
systemctl --user enable --now litellm
echo "Enabled and started litellm service"

# Show status
systemctl --user status litellm

echo ""
echo "=== Done ==="
echo "Logs: journalctl --user -u litellm -f"