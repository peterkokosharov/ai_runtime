#!/bin/bash
# install-service.sh - Install LiteLLM systemd user service
# Copyright (c) 2024 Peter Kokosharov <peter.kokosharov@gmail.com>
# LLM: NVIDIA Nemotron 3 Ultra <noreply@nvidia.com>
# Licensed under MIT License

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SERVICE_FILE="$SCRIPT_DIR/litellm.service"
USER_SYSTEMD_DIR="$HOME/.config/systemd/user"

echo "=== Installing LiteLLM systemd user service ==="
echo "Service file: $SERVICE_FILE"
echo "Target dir: $USER_SYSTEMD_DIR"

if [ ! -f "$SERVICE_FILE" ]; then
    echo "Error: $SERVICE_FILE not found"
    exit 1
fi

mkdir -p "$USER_SYSTEMD_DIR"
cp "$SERVICE_FILE" "$USER_SYSTEMD_DIR/"
echo "Copied service file"

systemctl --user daemon-reload
echo "Reloaded systemd"

systemctl --user enable --now litellm
echo "Enabled and started litellm service"

systemctl --user status litellm

echo ""
echo "=== Done ==="
echo "Logs: journalctl --user -u litellm -f"