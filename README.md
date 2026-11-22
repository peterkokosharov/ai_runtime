# ai_runtime — LLM runtime за LiteLLM и kiko

> Накратко: безплатни ключове от NVIDIA и Google → `kiko install` → `./run-claude-kiko.sh`.
> Няма такса, няма `npm`, няма sudo, няма `config.yaml` с ключове в него.

Тук има два начина да пуснеш локален gateway и да го управляваш от десктопа:

| | Gateway | Трай / панел | Къде е проверено |
|---|---|---|---|
| **kiko** (по-лекият, препоръчителен) | stdlib-only, `127.0.0.1:4000` | GTK3 на Linux Mint | Mint 22.3 |
| **LiteLLM** (старият, работи) | Python wheel + `~/.litellm/config.yaml` | TDE панел на Q4OS | Q4OS 6 / TDE R14 |

Documentation below follows the kiko path: Claude Code speaks the Anthropic API directly to kiko
through `run-claude-kiko.sh`. The old LiteLLM launcher (`run-claude.sh`) was replaced by it; the TDE
applet sources are still in `src/` (see the last section).

## Защо kiko, не LiteLLM

| | LiteLLM (старото) | kiko (сега) |
|---|---|---|
| Зависимости | wheel с FastAPI, uvicorn, pydantic, httpx... | само Python stdlib |
| Конфигурация | `~/.litellm/config.yaml` с ключове във файла | ключове в OS keyring, 13 алиаса в SQLite |
| API | OpenAI-compatible proxy | Anthropic `/v1/messages` (Claude Code работи директно) |
| Инсталация | `pipx`/`pip`, systemd service | `kiko install`, без sudo |
| Цена | Depends on the upstream provider | същите безплатни tier-ове на NVIDIA и Google — **няма такса за софтуер** |

Безплатно не значи неограничено: free tier-овете имат rate limits. Проверените алиаси по-долу
отговарят 200 през kiko; ключовете се вземат за минути и не изискват карта.

## 1. Безплатни API ключове

| Key | Откъде | Използва се за |
|---|---|---|
| `NVIDIA_API_KEY` | https://build.nvidia.com/ → sign in → API Keys | `nemotron-ultra`, `nemotron-super`, `llama-nemotron`, `muse-glimmer` |
| `GEMINI_API_KEY` | https://aistudio.google.com/apikey → Create API key | `gemini-lite`, `gemini-tutor` |

По избор: `MISTRAL_API_KEY` (https://console.mistral.ai/api-keys),
`OPENROUTER_API_KEY` (https://openrouter.ai/keys), `GROQ_API_KEY` (https://console.groq.com/keys).
`GITHUB_TOKEN` трябва само за push към GitHub. Ако някой ключ е качен в чат или git — отмени го и
извади нов. Никога не го слагай във файл.

## 2. Инсталирай kiko

```bash
sudo apt install -y libsecret-tools curl
git clone https://github.com/peterkokosharov/kiko.git ~/kiko
cd ~/kiko && bash tray/tools/kiko install
kiko doctor
curl -sS http://127.0.0.1:4000/health
# {"status":"ok"}
```

`kiko install` пита за ключовете с изключено ехо, слага 13-те проверени алиаса в `~/.kiko/kiko.db`,
създава systemd **user** unit `kiko-proxy.service` и пуска gateway-а. Без sudo.

Вариант с `.deb` (по-лесно за колеги):

```bash
cd ~/kiko
bash packaging/build-deb.sh
sudo apt install -y ./dist/kiko_0.3.0_all.deb
kiko install
```

За Mint/Ubuntu има отделен GTK3 tray пакет:

```bash
sudo apt install -y build-essential cmake libgtk-3-dev fakeroot
cd ~/kiko && bash packaging/build-tray-gtk3-deb.sh
sudo apt install -y ./dist/kiko-tray-gtk3_0.1.0_amd64.deb
```

Пълният guide, включително Q4OS и uninstall, е в `~/kiko/docs/INSTALL.md`.

## 3. Инсталирай Claude Code

```bash
curl -fsSL https://claude.ai/install.sh | bash
echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc
claude --version          # 2.1.330 при нас
```

Нативният инсталатор слага `claude` в `~/.local/bin`; не изисква root нито Node/npm.

## 4. Пусни Claude през kiko

```bash
git clone https://github.com/peterkokosharov/ai_runtime.git ~/ai_runtime
cd ~/ai_runtime
./run-claude-kiko.sh -p 'Reply with exactly: OK'   # → OK
./run-claude-kiko.sh                               # интерактивна сесия
```

`run-claude-kiko.sh` слага `ANTHROPIC_BASE_URL=http://127.0.0.1:4000`, dummy token и алиасите от
kiko базата. Проверява `/health` преди да стартира Claude и има fallback към `~/.local/bin/claude`,
ако `PATH` не е препрочетан.

### Кое име кой модел е

| Env var | kiko alias | Роля |
|---|---|---|
| `ANTHROPIC_MODEL` | `nemotron-ultra` | основен модел |
| `ANTHROPIC_DEFAULT_OPUS_MODEL` | `nemotron-ultra` | мениджър / оценка |
| `ANTHROPIC_DEFAULT_SONNET_MODEL` | `llama-nemotron` | основна разработка (най-чист отговор в сравнителния тест, 0.9 s) |
| `ANTHROPIC_DEFAULT_FABLE_MODEL` | `muse-glimmer` | външен одит |
| `ANTHROPIC_DEFAULT_HAIKU_MODEL` | `gemini-lite` | бързи прости задачи |

Не използвай LiteLLM имена като `nvidia/nemotron-ultra` — kiko ги приема за upstream id и връща 404.
Тук се ползват локалните алиаси от базата.

### Къде са ключовете

* В GNOME Keyring: `service=/peterk`, `key=<KEY>`, `username=<твоя логин>`, label `/peterk/<KEY>`.
* `ANTHROPIC_AUTH_TOKEN=sk-dummy-key` е само пълнител — Claude изисква променливата, kiko я игнорира.
* Токенът за kiko management API стои в `~/.kiko/api-token` с режим `0600`.
* Нито една стойност не влиза в git, `.env` или shell profile.

### Променливи, които значение имат

* `CLAUDE_CODE_DISABLE_UNKNOWN_MODEL_WINDOW_ENFORCEMENT=1` — **оставена**. Алиасите са непознати за
  Claude Code и той не знае контекста им; флагът го кара да компактира след реално „too long“, вместо
  на presume-ната граница.
* `LITELLM_DROP_PARAMS` — **махната**. LiteLLM-specific; kiko не я чете.
* `DISABLE_PROMPT_CACHING` — **махната**. kiko гради заявката от whitelist и маха `cache_control`
  преди провайдера, значи няма какво да се изключва.

Подробности и проверките от 2026-11-22 са в [`KIKO.md`](KIKO.md).

## Полезни команди

```bash
KIKO_PORT=5000 kiko install    # втори gateway на друг порт
kiko models                    # презареди алиасите, без да триеш базата
kiko restart                   # рестартирай user service-а
kiko uninstall                 # махни service и код; пази базата и ключовете
kiko uninstall --purge         # и базата; ключовете пак остават
```

Ако се появи `[claude-code:unrecognized_model] ...` — това е локално предупреждение на Claude Code
за непознат model id, не грешка от gateway-а. Отговорът пак идва през kiko.

## Старият TDE/LiteLLM applet (legacy)

`src/litellm_applet.*`, `src/litellm_process.*`, `litellm.service`, `config-sample.yaml`,
`install-service.sh` и `debian/` са от първоначалния проект за **Q4OS 6 / TDE R14**. Те управляват
LiteLLM през `~/.litellm/config.yaml` и не се използват в описания тук поток. Оставени са за
справка; не ги препоръчваме — kiko е по-лек и не държи ключове във файл. Ако ти трябва TDE панел,
в kiko има собствен `tray/tqt3/` (за Q4OS), но той не е build-ван на тази машина.

## Файлове в тоя repo

| Файл | Какво прави |
|---|---|
| `run-claude-kiko.sh` | единственият launcher — Claude Code → kiko |
| `KIKO.md` | model mapping, проверки, обяснение на env променливите |
| `kiko-proxy.service`, `install-kiko-service.sh` | по-старият checkout-базиран начин за същия service; `kiko install` е каноничен |
| `src/`, `litellm.service`, `debian/` | legacy TDE applet за LiteLLM |

## License

MIT — Copyright Peter Kokosharov <peter.kokosharov@gmail.com>
