#!/bin/bash
# install-service.sh - Install LiteLLM systemd user service
# Copyright (c) 2024 Peter Kokosharov <peter.kokosharov@gmail.com>
# LLM: NVIDIA Nemotron 3 Ultra <noreply@nvidia.com>
# Licensed under MIT License

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SERVICE_FILE="$SCRIPT_DIR/litellm.service"
USER_SYSTEMD_DIR="$HOME/.config/systemd/user"
LITELLM_DIR="$HOME/.litellm"

echo "=== Installing LiteLLM systemd user service ==="
echo "Service file: $SERVICE_FILE"
echo "Target dir: $USER_SYSTEMD_DIR"
echo "LiteLLM config dir: $LITELLM_DIR"

if [ ! -f "$SERVICE_FILE" ]; then
    echo "Error: $SERVICE_FILE not found"
    exit 1
fi

# Create ~/.litellm directory
mkdir -p "$LITELLM_DIR"

# Copy config sample if config.yaml doesn't exist
if [ ! -f "$LITELLM_DIR/config.yaml" ]; then
    cp "$SCRIPT_DIR/../config-sample.yaml" "$LITELLM_DIR/config.yaml"
    echo "Copied config-sample.yaml to $LITELLM_DIR/config.yaml"
    echo ">>> EDIT $LITELLM_DIR/config.yaml and add your API keys <<<"
else
    echo "Config already exists at $LITELLM_DIR/config.yaml"
fi

# Create env file if it doesn't exist
if [ ! -f "$LITELLM_DIR/env" ]; then
    cat > "$LITELLM_DIR/env" << 'EOF'
# LiteLLM API Keys - ADD YOUR KEYS HERE
# Get keys from:
#   Google Gemini: https://aistudio.google.com/apikey
#   NVIDIA Nemotron: https://integrate.api.nvidia.com
#   Groq: https://console.groq.com
#   OpenRouter: https://openrouter.ai/keys

# NVIDIA_API_KEY=your_key_here
# GOOGLE_API_KEY=your_key_here
# GROQ_API_KEY=your_key_here
# OPENROUTER_API_KEY=your_key_here
# ANTHROPIC_API_KEY=your_key_here
EOF
    echo "Created $LITELLM_DIR/env (add your API keys)"
else
    echo "Env file already exists at $LITELLM_DIR/env"
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
echo "Config: $LITELLM_DIR/config.yaml"
echo "Env:    $LITELLM_DIR/env"
echo "Logs:   journalctl --user -u litellm -f"