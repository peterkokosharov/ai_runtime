# LiteLLM TDE Panel Applet

TDE (Trinity Desktop Environment) panel applet for controlling LiteLLM proxy — native integration like battery/keyboard applets in the kicker panel.

## Quick Install (one command)

```bash
chmod +x install.sh && ./install.sh
```

This script installs dependencies, LiteLLM, builds the applet, installs it, and restarts the panel.

## Features

- **Native TDE panel applet** (KPanelApplet API) — loads inside kicker process
- **Status icon** in panel: green=running, red=error, gray=stopped, orange=starting
- **Right-click menu**: Start / Stop / Restart / Show Logs / Configure / Quit
- **Tooltip** with current status and port
- **Configuration**: Config file path, port, LiteLLM binary via TDE config system
- **Persistent settings** in `~/.trinity/share/config/litellmapplet.rc`

## Prerequisites (Q4OS 6 / Debian Trixie + TDE R14)

### 1. Install Build Tools (one-time)

```bash
sudo apt update
sudo apt install -y \
    cmake \
    tdelibs14-trinity-dev \
    tdebase-trinity-dev \
    libtqt3-mt-dev \
    tqt3-dev-tools
```

| Package | Purpose |
|---------|---------|
| `cmake` | Build system |
| `tdelibs14-trinity-dev` | TDE headers: `kapp.h`, `kpanelapplet.h`, `tdecore`, `tdeui` |
| `tdebase-trinity-dev` | Kicker panel development files |
| `libtqt3-mt-dev` | TQt3 (TDE's Qt3 fork) headers: `tqwidget.h`, `tqprocess.h` |
| `tqt3-dev-tools` | `tqmoc` (meta-object compiler for Qt3 signals/slots) |

### 2. Install LiteLLM (the proxy server)

The applet **controls** LiteLLM — install LiteLLM first:

```bash
# Recommended: pipx (isolated, auto-updates)
pipx install litellm

# Alternative: pip (user install)
pip install --user litellm

# Alternative: Docker
docker pull ghcr.io/berriai/litellm:main-latest
```

Verify:
```bash
litellm --version
# Example: 1.72.0
```

## Installation Order

### Step 1: Run LiteLLM as systemd Service (recommended)

This runs LiteLLM in the background, independent of the panel:

```bash
# Install service file (from project root)
./install-service.sh

# Enable + start on login
systemctl --user enable --now litellm

# Check status
systemctl --user status litellm
journalctl --user -u litellm -f  # follow logs
```

**Add API keys** (create `~/.config/litellm/env`):
```bash
OPENAI_API_KEY=sk-xxxxxxxxxxxx
ANTHROPIC_API_KEY=sk-ant-xxxxxxxxxxxx
```

### Step 2: Build & Install the Applet

```bash
cd ~/litellm-try
mkdir -p build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/opt/trinity
make -j$(nproc)
sudo make install
```

### Step 3: Restart Panel

```bash
dcop kicker Panel restart
```

### Quick One-Liner (after first install)

```bash
cd ~/litellm-try/build && sudo make install && dcop kicker Panel restart
```

## Usage

1. **Add to panel**: Right-click panel → Add → Applet → **LiteLLM Proxy**
2. **Configure**: Right-click applet → Configure → Select your `config.yaml`  
   (Default: `~/Documents/confg-kiko.yaml`)

## Project Structure

```
litellm-try/
├── CMakeLists.txt              # Build config
├── litellm-applet.desktop      # Kicker applet descriptor
├── README.md                   # This file
├── LICENSE                     # MIT License
├── install-service.sh          # Systemd service installer
├── litellm.service             # Systemd service template
├── github.md                   # GitHub publishing notes
├── icons/                      # 22x22 PNG icons (4 states)
└── src/
    ├── litellm_applet.h/.cpp   # Panel UI, menu, config
    └── litellm_process.h/.cpp  # Manages litellm subprocess
```

## Configuration

### Applet Settings (auto-saved in TDE config)

File: `~/.trinity/share/config/litellmapplet.rc`

```ini
[LiteLLMApplet]
ConfigPath=/home/user/Documents/confg-kiko.yaml
BinaryPath=litellm
Port=4000
```

### LiteLLM Config

**Use the sample file as starting point:**

```bash
cp config-sample.yaml ~/Documents/confg-kiko.yaml
# Then edit ~/Documents/confg-kiko.yaml and add your API keys
```

The `config-sample.yaml` includes examples for:
- NVIDIA Nemotron 3 Ultra / Super
- Google Gemini Flash / Flash Lite
- Anthropic Claude
- Groq Llama
- OpenRouter Qwen Coder

See [LiteLLM Config Docs](https://docs.litellm.ai/docs/proxy/configs) for full options.

### API Keys — Two Options

**Option 1: Env file (recommended for systemd service)**
- Keys stay out of config files
- Systemd service loads them automatically via `EnvironmentFile`

```bash
# Create env file
cat > ~/.config/litellm/env << 'EOF'
NVIDIA_API_KEY=your_nvidia_key
GOOGLE_API_KEY=your_google_key
ANTHROPIC_API_KEY=your_anthropic_key
GROQ_API_KEY=your_groq_key
OPENROUTER_API_KEY=your_openrouter_key
EOF
```

In `config-sample.yaml` use placeholders:
```yaml
api_key: "YOUR_NVIDIA_API_KEY_HERE"  # Will be replaced from env at runtime
```

**Option 2: Direct in YAML with `os.environ/`**
- Portable config, references env vars by name
- LiteLLM expands `os.environ/VAR_NAME` at startup

```yaml
api_key: os.environ/NVIDIA_API_KEY
```

Then export in shell or add to `~/.profile`:
```bash
export NVIDIA_API_KEY=your_key
```

> **Tip:** Option 1 is cleaner for systemd. Option 2 works well if running LiteLLM manually from terminal.

## Service Management

```bash
# Start / Stop / Restart
systemctl --user start litellm
systemctl --user stop litellm
systemctl --user restart litellm

# Status + logs
systemctl --user status litellm
journalctl --user -u litellm -f

# Disable
systemctl --user disable --now litellm
```

> **Note:** If using systemd service, the applet's Start/Stop buttons won't control the process (they manage direct subprocess). Update `src/litellm_process.cpp` to use `systemctl --user` commands instead.

## Rebuild After Changes

```bash
cd ~/litellm-try/build
make -j$(nproc)
sudo make install
dcop kicker Panel restart
```

## Troubleshooting

| Issue | Fix |
|-------|-----|
| Applet not in "Add Applet" list | `kbuildsycoca --incremental` or re-login |
| Icons missing | Check `/opt/trinity/share/icons/hicolor/22x22/apps/litellm_*.png` |
| Build fails: `kpanelapplet.h` not found | `sudo apt install tdelibs14-trinity-dev` |
| Build fails: `tqwidget.h` not found | `sudo apt install libtqt3-mt-dev` |
| Build fails: `tqmoc` not found | `sudo apt install tqt3-dev-tools` |
| LiteLLM won't start | Check `BinaryPath` in config; ensure `litellm` in PATH |
| "Library files for litellm_applet.la not found" | CMake creates `.la` on install. If missing: `sudo cp /tmp/litellm_applet.la /opt/trinity/lib/trinity/` |

## How It Works

```
Kicker Panel
  └── LiteLLMApplet (panel UI: icon + menu)
        └── LiteLLMProcess (manages litellm subprocess)
              └── litellm binary (external process)
```

- **LiteLLMApplet** — Panel integration, icon, menu, config dialog
- **LiteLLMProcess** — Starts/stops `litellm`, reads stdout/stderr, emits Qt signals

### TDE/Qt3 (TQt3) Notes

- Uses **TQt3** (TDE's Qt3 fork): `TQString`, `TQProcess`, `TQTimer`, `TQIconSet`
- Signals/slots: `TQ_SIGNAL()`, `TQ_SLOT()` macros
- `TQProcess` uses `canReadLineStdout()` / `readLineStdout()`
- String trim: `stripWhiteSpace()` (not `trimmed()`)
- MOC: `tqmoc` (not standard `moc`)

## License

MIT — see [LICENSE](LICENSE) file.