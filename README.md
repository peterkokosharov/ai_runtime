# LiteLLM TDE Panel Applet

TDE (Trinity Desktop Environment) panel applet for controlling LiteLLM proxy — native integration like battery/keyboard applets in the kicker panel.

> **For Claude Code, use the kiko section below.** The rest of this README describes the older
> LiteLLM/TDE setup. kiko-lite speaks the same Anthropic API on port 4000 but does not use LiteLLM,
> a `config.yaml`, or the TDE applet.

## Claude Code + kiko-lite (verified on Linux Mint 22.3)

Four things are needed: **API keys in the OS keyring**, the **kiko gateway**, the **Claude Code CLI**,
and this repo's **`run-claude-kiko.sh`**. Nothing below writes a key to disk or to git.

**Fast path** (keys from step 1, then one command; manual steps follow):

```bash
git clone -b fix/mint-tray-build https://github.com/peterkokosharov/kiko.git ~/kiko
cd ~/kiko && bash tray/tools/kiko install
kiko doctor
```

`kiko install` asks for the keys with echo off, loads the 13 verified model aliases, writes a systemd
**user** unit and starts the gateway. For a `.deb` instead of a checkout, see `docs/INSTALL.md` in the
kiko repo.

### 1. Get free API keys

| Key | Where | Used for |
|---|---|---|
| `NVIDIA_API_KEY` | https://build.nvidia.com/ → sign in → API Keys | `nemotron-ultra`, `nemotron-super`, `muse-glimmer` and the other NVIDIA models |
| `GEMINI_API_KEY` | https://aistudio.google.com/apikey → Create API key | `gemini-lite`, `gemini-tutor` (fast/simple work) |

Optional, only if you want those providers: `MISTRAL_API_KEY` (https://console.mistral.ai/api-keys),
`OPENROUTER_API_KEY` (https://openrouter.ai/keys), `GROQ_API_KEY` (https://console.groq.com/keys).
`GITHUB_TOKEN` is only needed to push commits. On this machine Groq answers 403, so no Groq models
are configured. If a key is pasted anywhere public, revoke it and make a new one.

### 2. Store the keys in GNOME Keyring

```bash
sudo apt install -y libsecret-tools          # provides secret-tool
git clone -b fix/mint-tray-build https://github.com/peterkokosharov/kiko.git ~/kiko
cd ~/kiko
bash tray/tools/setup-keys.sh
```

The script asks for each key with echo turned off; press Enter to skip one. It stores them under
`service=/peterk`, `key=<KEY>`, `username=<your login>`, label `/peterk/<KEY>`. Verify **without
printing the value**:

```bash
secret-tool lookup service /peterk key NVIDIA_API_KEY username "$USER" >/dev/null && echo ok
secret-tool lookup service /peterk key GEMINI_API_KEY  username "$USER" >/dev/null && echo ok
```

Never put a key in `.env`, `config.yaml`, a shell profile, or git. The scripts use your current login
as the keyring username, so no editing is needed on another laptop.

### 3. Start the gateway and install the verified model catalog

`kiko install` in the fast path already did both steps. The manual equivalent is:

```bash
cd ~/kiko
bash tray/tools/setup-models.sh              # adds 13 tested aliases; key NAMES only
```

Then run the gateway either in the foreground **or** as a user service — not both (port 4000).
If you used `kiko install`, skip this: it already enabled `kiko-proxy.service`. The two files
`kiko-proxy.service` / `install-kiko-service.sh` in this repo are the older, checkout-based way to do
the same thing; `kiko install` from the kiko repo is now the canonical installer.

```bash
# Option A: foreground, useful for debugging
bash ~/kiko/tray/tools/run-kiko.sh

# Option B: starts on login, no sudo (recommended)
cd ~/ai_runtime && ./install-kiko-service.sh
systemctl --user status kiko-proxy.service
```

`kiko-proxy.service` waits for the graphical session, checks that the NVIDIA and Gemini keys are
visible in the keyring, and retries every 5 s if the keyring is still locked. Use `journalctl --user
-u kiko-proxy -f` for logs. In another terminal:

```bash
curl -sS http://127.0.0.1:4000/health
# {"status":"ok"}
```

The gateway exports the keys into its own process only; it never writes them to a file.

### 4. Install Claude Code (no sudo, no Node needed)

```bash
curl -fsSL https://claude.ai/install.sh | bash
echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc
claude --version
```

The native installer puts `claude` in `~/.local/bin`. It does not require root or npm.

### 5. Run Claude through kiko

```bash
git clone https://github.com/peterkokosharov/ai_runtime.git ~/ai_runtime
cd ~/ai_runtime
./run-claude-kiko.sh -p 'Reply with exactly: OK'   # should print OK
./run-claude-kiko.sh                               # interactive session
```

This repo may be private; use your GitHub credentials to clone it. `run-claude-kiko.sh` sets
`ANTHROPIC_BASE_URL=http://127.0.0.1:4000`, a dummy auth token, and the model aliases. The real
provider keys stay in the keyring and the gateway's own token stays in `~/.kiko/api-token` (0600).
See `KIKO.md` for the exact model mapping, the benchmark that picked `llama-nemotron` for sonnet,
and which environment variables were kept or dropped.

On the other laptops use the matching script from `~/kiko/tray/tools/build-*.sh` and fill in your row
in `~/kiko/docs/BUILD-MATRIX.md`.

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

**The installer creates `~/.litellm/config.yaml` and `~/.litellm/env`** — edit them to add your API keys.

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
2. **Configure**: Edit `~/.litellm/config.yaml` and add your API keys

## Project Structure

```
litellm-try/
├── CMakeLists.txt              # Build config
├── litellm-applet.desktop      # Kicker applet descriptor
├── README.md                   # This file
├── KIKO.md                     # Claude Code ↔ kiko model mapping and checks
├── run-claude-kiko.sh          # Claude Code launcher for the kiko gateway
├── kiko-proxy.service          # systemd user unit for the kiko gateway
├── install-kiko-service.sh     # installs kiko-proxy.service (no sudo)
├── LICENSE                     # MIT License
├── install-service.sh          # LiteLLM systemd service installer
├── litellm.service             # LiteLLM systemd service template
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
# Option A: Manual setup
mkdir -p ~/.litellm
cp config-sample.yaml ~/.litellm/config.yaml
# Edit ~/.litellm/config.yaml and add your API keys

# Option B: Use install-service.sh (does this automatically)
./install-service.sh
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

**Create config and env files:**
```bash
mkdir -p ~/.litellm
cp config-sample.yaml ~/.litellm/config.yaml
cat > ~/.litellm/env << 'EOF'
# LiteLLM API Keys - ADD YOUR KEYS HERE
GOOGLE_API_KEY=your_gemini_key_here
NVIDIA_API_KEY=your_nvidia_key_here
GROQ_API_KEY=your_groq_key_here
OPENROUTER_API_KEY=your_openrouter_key_here
# ANTHROPIC_API_KEY=your_key_here
EOF
chmod 600 ~/.litellm/env
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
# This sets all env vars automatically. It points Claude Code at the local
# kiko-lite gateway (http://127.0.0.1:4000); see KIKO.md for the model mapping.
./run-claude-kiko.sh
# Or if installed via symlink:
run-claude-kiko.sh
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
| "401 Unauthorized" | Check API keys in `~/.litellm/env` |
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
cp /usr/share/doc/litellm-tde-applet/config-sample.yaml ~/.litellm/config.yaml
# Edit ~/.litellm/config.yaml with your keys
# Also edit ~/.litellm/env and add your API keys

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