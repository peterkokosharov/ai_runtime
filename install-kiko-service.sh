#!/usr/bin/env bash
# Install the kiko-lite gateway as a systemd *user* service (no sudo).
# Usage: bash install-kiko-service.sh
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
DEST="$HOME/.config/systemd/user/kiko-proxy.service"

if [ ! -d "$HOME/kiko/gateway" ]; then
    echo "kiko not found at ~/kiko — clone it first:" >&2
    echo "  git clone https://github.com/peterkokosharov/kiko.git ~/kiko" >&2
    exit 1
fi
if ! command -v secret-tool >/dev/null 2>&1; then
    echo "secret-tool missing — install it with: sudo apt install libsecret-tools" >&2
    exit 1
fi

mkdir -p "$HOME/.config/systemd/user"
cp "$ROOT/kiko-proxy.service" "$DEST"
systemctl --user daemon-reload
systemctl --user enable --now kiko-proxy.service
echo "kiko-proxy installed and started"
echo ""
systemctl --user status kiko-proxy.service --no-pager || true
echo ""
echo "Check:  curl -sS http://127.0.0.1:4000/health"
echo "Logs:   journalctl --user -u kiko-proxy -f"
