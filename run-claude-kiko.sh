#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Launch Claude Code against the local kiko-lite gateway (Anthropic-compatible /v1/messages).
# Replaced the old LiteLLM-only run-claude.sh; this is now the only launcher.
# Usage: bash ~/ai_runtime/run-claude-kiko.sh [claude args...]
set -euo pipefail

# kiko serves the Anthropic API on the same port as the tray and the web page.
export ANTHROPIC_BASE_URL="http://127.0.0.1:4000"
# Claude Code insists on a token; kiko's gateway ignores it. The gateway's own /kiko/v1/*
# management token stays in ~/.kiko/api-token and is never used here.
export ANTHROPIC_AUTH_TOKEN="sk-dummy-key"
# No LITELLM_DROP_PARAMS: kiko is not LiteLLM and never reads that variable.

# Model aliases from kiko's database (verified through /v1/messages on 2026-11-22).
# Do NOT use LiteLLM-style names here: kiko resolves nvidia/nemotron-ultra as an
# upstream NVIDIA id and gets a 404. These are the local aliases instead.
export ANTHROPIC_MODEL="nemotron-ultra"
export ANTHROPIC_DEFAULT_HAIKU_MODEL="gemini-lite"
export ANTHROPIC_DEFAULT_OPUS_MODEL="nemotron-ultra"
export ANTHROPIC_DEFAULT_SONNET_MODEL="llama-nemotron"
export ANTHROPIC_DEFAULT_FABLE_MODEL="muse-glimmer"

# Our aliases are unknown to Claude Code, so it cannot know their real context window.
# Keep this: it makes Claude compact after an API "too long" rejection instead of
# guessing. No DISABLE_PROMPT_CACHING: kiko whitelists request fields and already
# drops cache_control before the upstream call, so there is nothing to disable.
export CLAUDE_CODE_DISABLE_UNKNOWN_MODEL_WINDOW_ENFORCEMENT=1

if ! command -v curl >/dev/null 2>&1 || ! curl -fsS --max-time 3 "$ANTHROPIC_BASE_URL/health" >/dev/null 2>&1; then
    echo "kiko: gateway is not answering at $ANTHROPIC_BASE_URL" >&2
    echo 'kiko: start it with: bash "$HOME/Documents/Default Project/kiko/tray/tools/run-kiko.sh"' >&2
    exit 1
fi
CLAUDE_BIN="$(command -v claude 2>/dev/null || true)"
if [ -z "$CLAUDE_BIN" ] && [ -x "$HOME/.local/bin/claude" ]; then
    CLAUDE_BIN="$HOME/.local/bin/claude"
fi
if [ -z "$CLAUDE_BIN" ]; then
    echo "kiko: 'claude' (Claude Code CLI) not found (looked in PATH and ~/.local/bin)" >&2
    exit 1
fi

exec "$CLAUDE_BIN" "$@"
