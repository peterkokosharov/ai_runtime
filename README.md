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
   (Default: `~/Documents/config-kiko.yaml`)

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
ConfigPath=/home/user/Documents/config-kiko.yaml
BinaryPath=litellm
Port=4000
```

### LiteLLM Config

**Use the sample file as starting point:**

```bash
cp config-sample.yaml ~/Documents/config-kiko.yaml
# Then edit ~/Documents/config-kiko.yaml and add your API keys
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

## Complete Free Setup for Students

This setup lets you run an AI coding assistant **completely free** using free-tier APIs.

### Prerequisites
- Linux (Q4OS, Debian, Ubuntu, Mint, etc.) with TDE/Trinity Desktop
- Internet connection for API access

---

### Step 1: Get Free API Keys (5 minutes)

| Provider | Free Tier | Get Key |
|----------|-----------|---------|
| **Google Gemini** | 1500 req/day, 1M tokens/min | [aistudio.google.com/apikey](https://aistudio.google.com/apikey) |
| **NVIDIA Nemotron** | Generous free tier | [integrate.api.nvidia.com](https://integrate.api.nvidia.com) |
| **Groq** | 14400 req/day | [console.groq.com](https://console.groq.com) |
| **OpenRouter** | Free models available | [openrouter.ai/keys](https://openrouter.ai/keys) |

**Create env file:**
```bash
mkdir -p ~/.config/litellm
cat > ~/.config/litellm/env << 'EOF'
GOOGLE_API_KEY=your_gemini_key_here
NVIDIA_API_KEY=your_nvidia_key_here
GROQ_API_KEY=your_groq_key_here
OPENROUTER_API_KEY=your_openrouter_key_here
EOF
chmod 600 ~/.config/litellm/env
```

---

### Step 2: Install & Configure LiteLLM Proxy

```bash
# Install LiteLLM
pipx install litellm

# Copy and edit config
cp config-sample.yaml ~/Documents/config-kiko.yaml
# Edit ~/Documents/config-kiko.yaml if needed (defaults work)

# Install systemd service (runs proxy in background)
./install-service.sh
systemctl --user enable --now litellm

# Verify it's running
systemctl --user status litellm
# Should show: Active: active (running)
```

The proxy now runs on `http://127.0.0.1:4000` and routes requests to your free APIs.

---

### Step 3: Install This Applet (Panel Control)

```bash
# Build and install
chmod +x install.sh && ./install.sh

# Or manually:
mkdir -p build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/opt/trinity
make -j$(nproc)
sudo make install
dcop kicker Panel restart
```

**Add to panel:** Right-click panel → Add → Applet → **LiteLLM Proxy**

---

### Step 4: Connect Any Anthropic-Compatible Client

The proxy speaks the **Anthropic API format**. Point any compatible tool to `http://127.0.0.1:4000`.

#### Option A: Official Claude Code (requires paid Anthropic plan)
```bash
# If you have Claude Code access:
export ANTHROPIC_BASE_URL="http://127.0.0.1:4000"
export ANTHROPIC_AUTH_TOKEN="sk-dummy-key"  # not used by LiteLLM
claude
```

#### Option B: Free/Open Source Clients (recommended for students)

**claude-code-oss / similar wrappers:**
```bash
# Many community wrappers exist that work with Anthropic-compatible endpoints
# Search GitHub for "claude-code" + "anthropic-base-url"
```

**Any HTTP client (curl, Python, etc.):**
```bash
# Test the proxy directly
curl -X POST http://127.0.0.1:4000/v1/messages \
  -H "Content-Type: application/json" \
  -H "x-api-key: sk-dummy" \
  -d '{
    "model": "nvidia/nemotron-ultra",
    "max_tokens": 100,
    "messages": [{"role": "user", "content": "Hello!"}]
  }'
```

#### Option C: Use the included launcher
```bash
# This sets all env vars automatically
./run-claude.sh
# Or if installed via symlink:
run-claude.sh
```

---

### How It Works (Summary)

```
Your Editor/CLI
       │
       ▼ (Anthropic API format)
LiteLLM Proxy (port 4000)
       │
       ├──▶ Google Gemini (free)
       ├──▶ NVIDIA Nemotron (free)
       ├──▶ Groq Llama (free)
       └──▶ OpenRouter (free models)
```

**You pay $0.** The proxy routes each request to whichever free API you configured.

---

### Troubleshooting Free Setup

| Problem | Solution |
|---------|----------|
| "401 Unauthorized" | Check API keys in `~/.config/litellm/env` |
| "Model not found" | Use model names from `config-sample.yaml` (e.g., `nvidia/nemotron-ultra`) |
| Proxy not starting | `journalctl --user -u litellm -f` for logs |
| Panel applet not showing | `dcop kicker Panel restart` or re-login |

---

## Building .deb Package (Q4OS / Debian)

This project includes a `debian/` directory for building a native `.deb` package.

### Quick Build

```bash
# Install build tools
sudo apt install -y devscripts debhelper dh-make build-essential

# Build package (run from project root)
dpkg-buildpackage -us -uc -b

# Result: ../litellm-tde-applet_1.0.0-1_amd64.deb
```

### Install .deb

```bash
sudo apt install ../litellm-tde-applet_1.0.0-1_amd64.deb
```

**What the package does automatically:**
- Installs applet to `/opt/trinity/lib/trinity/`
- Installs desktop entry to `/opt/trinity/share/apps/kicker/applets/`
- Installs icons to `/opt/trinity/share/icons/hicolor/22x22/apps/`
- Installs config sample to `/usr/share/doc/litellm-tde-applet/config-sample.yaml`
- Installs LiteLLM via `pipx` (for current user)
- Sets up systemd user service for LiteLLM proxy
- Restarts kicker panel

### After Install

```bash
# 1. Add applet to panel
Right-click panel → Add → Applet → LiteLLM Proxy

# 2. Copy config and add API keys
cp /usr/share/doc/litellm-tde-applet/config-sample.yaml ~/Documents/config-kiko.yaml
# Edit ~/Documents/config-kiko.yaml with your keys

# 3. Check LiteLLM service
systemctl --user status litellm
```

### For Q4OS Maintainers

To add to Q4OS repositories:

1. **Source package upload:**
   ```bash
   dput q4os ../litellm-tde-applet_1.0.0-1_source.changes
   ```

2. **Build dependencies** (in `debian/control`):
   - `cmake`, `tdelibs14-trinity-dev`, `tdebase-trinity-dev`
   - `libtqt3-mt-dev`, `tqt3-dev-tools`
   - `python3-pipx`, `python3-venv`

3. **Runtime dependencies:**
   - `libtqt3-mt`, `tdelibs14-trinity`
   - `python3-pipx` (for LiteLLM auto-updates)

4. **Tested on:** Q4OS 6 (Trinity/Trixie), TDE R14

### Package Structure

```
debian/
├── control          # Package metadata & dependencies
├── rules            # Build rules (CMake + dh)
├── install          # File installation paths
├── postinst         # Post-install: pipx install, service setup, panel restart
├── prerm            # Pre-remove: stop service, restart panel
├── changelog        # Version history
├── compat           # Debhelper compatibility (13)
├── install-service.sh  # Systemd service installer
└── litellm.service     # Systemd user service template
```

> **Note:** This package is for **Q4OS with TDE/Trinity Desktop**. It installs to `/opt/trinity/` paths and uses TDE libraries. Not compatible with KDE Plasma or GNOME.