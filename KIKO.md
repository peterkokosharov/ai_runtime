# kiko ↔ Claude Code

The old LiteLLM-only launcher (`run-claude.sh`, with names like `nvidia/nemotron-ultra`) was removed.
`kiko-lite` is a different gateway: it speaks the Anthropic API on the same `http://127.0.0.1:4000`,
but resolves **its own database aliases**.

Use `run-claude-kiko.sh`:

```bash
bash ~/ai_runtime/run-claude-kiko.sh
# optional: alias claude='~/ai_runtime/run-claude-kiko.sh'
```

It exports:

| Env var | kiko alias | Role |
|---|---|---|
| `ANTHROPIC_MODEL` | `nemotron-ultra` | default model |
| `ANTHROPIC_DEFAULT_HAIKU_MODEL` | `gemini-lite` | fast/simple work |
| `ANTHROPIC_DEFAULT_OPUS_MODEL` | `nemotron-ultra` | manager / evaluator |
| `ANTHROPIC_DEFAULT_SONNET_MODEL` | `llama-nemotron` | main implementation (benchmark winner: clean answer in 0.9 s) |
| `ANTHROPIC_DEFAULT_FABLE_MODEL` | `muse-glimmer` | external auditor |

`ANTHROPIC_AUTH_TOKEN` is a placeholder (`sk-dummy-key`): Claude Code requires the variable, kiko
does not use it. Keep provider keys in the OS keyring and the gateway's management token in
`~/.kiko/api-token` (0600); no key value belongs in this repo.

Verified on 2026-11-22 against the running gateway: non-streaming `POST /v1/messages`, streaming
`POST /v1/messages?stream=true`, and `POST /v1/messages/count_tokens` all return 200 for every
alias above.

## Prerequisite

The launcher needs the Claude Code CLI (`claude`) on `PATH`, plus a running gateway:

```bash
bash "/home/peterk/Documents/Default Project/kiko/tray/tools/run-kiko.sh"
```

Claude Code 2.1.330 is installed natively in `~/.local/bin/claude` (no Node/npm needed); that path is
added to `PATH` in `~/.bashrc`. The launcher also falls back to the absolute path, so it works even
in a shell that has not re-read `.bashrc`. End-to-end check on 2026-11-22:

```bash
cd ~/ai_runtime && ./run-claude-kiko.sh -p 'Reply with exactly: OK'
# [claude-code:unrecognized_model] {"model":"nemotron-ultra","query_source":"sdk"}
# OK
```

The `unrecognized_model` line is Claude Code's local warning about a non-Anthropic model id; it is
not a gateway error and the answer still comes through kiko.

## Which environment variables are actually needed

* `LITELLM_DROP_PARAMS` — **removed**. It is a LiteLLM-only switch; kiko never reads it.
* `DISABLE_PROMPT_CACHING` — **removed**. kiko builds upstream requests from a field whitelist and
  drops `cache_control` (and beta headers) before they reach a provider, so no caching happens anyway.
* `CLAUDE_CODE_DISABLE_UNKNOWN_MODEL_WINDOW_ENFORCEMENT=1` — **kept**. All our aliases are unknown to
  Claude Code, so it cannot know their context window. This makes it compact after an API "too long"
  rejection instead of forcing compaction at a guessed boundary. For `nemotron-ultra` the guess is
  actually 1M (the model's real window); for the smaller aliases it can be wrong, which is why the
  switch stays as a safety net.
