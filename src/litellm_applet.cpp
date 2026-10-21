/*
 * LiteLLM TDE Panel Applet
 * Copyright (c) 2024 Peter Kokosharov <peter.kokosharov@gmail.com>
 * LLM: NVIDIA Nemotron 3 Ultra <noreply@nvidia.com>
 * Licensed under MIT License
 */

#include "litellm_applet.h"
#include <tdeconfig.h>
#include <tdeglobal.h>
#include <tdelocale.h>
#include <tdemessagebox.h>
#include <tdefiledialog.h>
#include <kstandarddirs.h>
#include <tqlayout.h>
#include <tqtooltip.h>
#include <tqcursor.h>
#include <kiconloader.h>
#include <tdeapplication.h>
#include <kdebug.h>
#include <unistd.h>

extern "C" {
    KPanelApplet* init(TQWidget* parent, const TQValueList<TQVariant>& args) {
        TQString configFile;
        if (args.count() > 0) {
            configFile = args[0].toString();
        }
        return new LiteLLMApplet(configFile, parent);
    }
}

// tdeinit entry point - required for TDE panel applets (must be outside extern "C" for visibility)
extern "C" TDE_EXPORT void tdeinit_litellm_applet() {
    // Empty - presence is what matters for tdeinit loading mechanism
}

LiteLLMApplet::LiteLLMApplet(const TQString& configFile, TQWidget* parent)
    : KPanelApplet(configFile, KPanelApplet::Normal, 0, parent, "litellm_applet")
    , m_process(new LiteLLMProcess(this))
    , m_menu(new TQPopupMenu(this))
    , m_tooltipTimer(new TQTimer(this))
    , m_iconLabel(new TQLabel(this))
    , m_port(4000)
{
    // Load config
    setupConfig();

    // Load icons from TDE icon system
    m_iconRunning   = TDEGlobal::iconLoader()->loadIconSet("litellm_running", TDEIcon::Panel);
    m_iconStopped   = TDEGlobal::iconLoader()->loadIconSet("litellm_stopped", TDEIcon::Panel);
    m_iconError     = TDEGlobal::iconLoader()->loadIconSet("litellm_error", TDEIcon::Panel);
    m_iconStarting  = TDEGlobal::iconLoader()->loadIconSet("litellm_idle", TDEIcon::Panel);
    m_iconIdle      = m_iconStopped;  // fallback

    // Fallback: if theme icons not found, try loading from installed path
    if (m_iconRunning.isNull()) {
        TQString iconDir = TDEGlobal::dirs()->findResource("icon", "hicolor/22x22/apps/litellm_running.png");
        if (!iconDir.isEmpty()) {
            m_iconRunning   = TQIconSet(TQPixmap(iconDir));
            m_iconStopped   = TQIconSet(TQPixmap(TDEGlobal::dirs()->findResource("icon", "hicolor/22x22/apps/litellm_stopped.png")));
            m_iconError     = TQIconSet(TQPixmap(TDEGlobal::dirs()->findResource("icon", "hicolor/22x22/apps/litellm_error.png")));
            m_iconStarting  = TQIconSet(TQPixmap(TDEGlobal::dirs()->findResource("icon", "hicolor/22x22/apps/litellm_idle.png")));
            m_iconIdle      = m_iconStopped;
        }
    }

    // UI setup - in Qt3, layout is auto-set when created with parent
    TQHBoxLayout* layout = new TQHBoxLayout(this);
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(m_iconLabel);
    m_iconLabel->setPixmap(m_iconStopped.pixmap(TQIconSet::Small, TQIconSet::Normal));
    // No setLayout() needed in Qt3 - parent constructor sets it

    // Process signals
    connect(m_process, TQ_SIGNAL(stateChanged(LiteLLMProcess::State)), this, TQ_SLOT(onProcessStateChanged(LiteLLMProcess::State)));
    connect(m_process, TQ_SIGNAL(outputReceived(const TQString&)), this, TQ_SLOT(onProcessOutput(const TQString&)));
    connect(m_process, TQ_SIGNAL(errorOccurred(const TQString&)), this, TQ_SLOT(onProcessError(const TQString&)));

    // Tooltip update timer
    connect(m_tooltipTimer, TQ_SIGNAL(timeout()), this, TQ_SLOT(updateTooltip()));
    m_tooltipTimer->start(5000);

    // Context menu
    createMenu();
    setCustomMenu(m_menu);

    // Apply config to process
    m_process->setConfigPath(m_configPath);
    m_process->setPort(m_port);
    m_process->setLiteLLMBinary(m_binaryPath);

    // Initial state
    updateIcon();
    updateTooltip();

    kdDebug() << "LiteLLM Applet constructed" << endl;
}

LiteLLMApplet::~LiteLLMApplet() {
    if (m_process->state() != LiteLLMProcess::Stopped) {
        m_process->stop();
    }
}

int LiteLLMApplet::widthForHeight(int h) {
    return h;  // square applet
}

int LiteLLMApplet::heightForWidth(int w) {
    return w;
}

void LiteLLMApplet::createMenu() {
    m_menu->clear();

    m_menu->insertItem(i18n("Start"), this, TQ_SLOT(onStart()));
    m_menu->insertItem(i18n("Stop"), this, TQ_SLOT(onStop()));
    m_menu->insertItem(i18n("Restart"), this, TQ_SLOT(onRestart()));
    m_menu->insertSeparator();
    m_menu->insertItem(i18n("Show Logs"), this, TQ_SLOT(onShowLogs()));
    m_menu->insertSeparator();
    m_menu->insertItem(i18n("Configure..."), this, TQ_SLOT(configure()));
    m_menu->insertSeparator();
    m_menu->insertItem(i18n("Quit Panel Applet"), kapp, TQ_SLOT(quit()));
}

void LiteLLMApplet::setupConfig() {
    TDEConfig* config = TDEGlobal::config();
    config->setGroup("LiteLLMApplet");

    m_configPath = config->readPathEntry("ConfigPath", "");
    m_binaryPath = config->readEntry("BinaryPath", "litellm");
    m_port = config->readNumEntry("Port", 4000);

    // Default config path if empty
    if (m_configPath.isEmpty()) {
        TQString home = getenv("HOME");
        m_configPath = home + "/Documents/confg-kiko.yaml";
    }
}

void LiteLLMApplet::configure() {
    // Simple config dialog using KMessageBox + KFileDialog
    TQString newConfig = KFileDialog::getOpenFileName(
        m_configPath, "*.yaml", this,
        i18n("Select LiteLLM Config File")
    );

    if (!newConfig.isEmpty()) {
        m_configPath = newConfig;
        m_process->setConfigPath(m_configPath);

        TDEConfig* config = TDEGlobal::config();
        config->setGroup("LiteLLMApplet");
        config->writePathEntry("ConfigPath", m_configPath);
        config->sync();
    }
}

TQString LiteLLMApplet::configFilePath() const {
    return m_configPath;
}

void LiteLLMApplet::onStart() {
    if (m_process->state() == LiteLLMProcess::Stopped) {
        m_process->start();
    }
}

void LiteLLMApplet::onStop() {
    if (m_process->state() != LiteLLMProcess::Stopped &&
        m_process->state() != LiteLLMProcess::Stopping) {
        m_process->stop();
    }
}

void LiteLLMApplet::onRestart() {
    if (m_process->state() != LiteLLMProcess::Stopped) {
        m_process->stop();
    }
    // Restart after brief delay
    TQTimer::singleShot(500, this, TQ_SLOT(onStart()));
}

void LiteLLMApplet::onShowLogs() {
    // For now, just show a message box with recent output
    // TODO: proper log viewer window
    KMessageBox::information(this,
        i18n("Log viewer not implemented yet.\nCheck terminal where panel was started."),
        i18n("LiteLLM Logs")
    );
}

void LiteLLMApplet::onProcessStateChanged(LiteLLMProcess::State state) {
    updateIcon();
    updateTooltip();

    // Update menu item enabled states
    bool isRunning = (state == LiteLLMProcess::Running);
    bool isStopped = (state == LiteLLMProcess::Stopped);
    bool isBusy = (state == LiteLLMProcess::Starting || state == LiteLLMProcess::Stopping);

    m_menu->setItemEnabled(0, !isRunning && !isBusy);  // Start
    m_menu->setItemEnabled(1, isRunning && !isBusy);   // Stop
    m_menu->setItemEnabled(2, isRunning && !isBusy);   // Restart
}

void LiteLLMApplet::onProcessOutput(const TQString& line) {
    kdDebug() << "LiteLLM:" << line << endl;
}

void LiteLLMApplet::onProcessError(const TQString& msg) {
    KMessageBox::error(this, msg, i18n("LiteLLM Error"));
}

void LiteLLMApplet::updateIcon() {
    TQPixmap pix;
    switch (m_process->state()) {
        case LiteLLMProcess::Running:
            pix = m_iconRunning.pixmap(TQIconSet::Small, TQIconSet::Normal);
            break;
        case LiteLLMProcess::Starting:
            pix = m_iconStarting.pixmap(TQIconSet::Small, TQIconSet::Normal);
            break;
        case LiteLLMProcess::Error:
            pix = m_iconError.pixmap(TQIconSet::Small, TQIconSet::Normal);
            break;
        case LiteLLMProcess::Stopping:
            pix = m_iconIdle.pixmap(TQIconSet::Small, TQIconSet::Normal);
            break;
        default:
            pix = m_iconStopped.pixmap(TQIconSet::Small, TQIconSet::Normal);
            break;
    }
    m_iconLabel->setPixmap(pix);
}

void LiteLLMApplet::updateTooltip() {
    TQString tooltip;
    switch (m_process->state()) {
        case LiteLLMProcess::Running:
            tooltip = i18n("LiteLLM Proxy: Running on port %1").arg(m_port);
            break;
        case LiteLLMProcess::Starting:
            tooltip = i18n("LiteLLM Proxy: Starting...");
            break;
        case LiteLLMProcess::Stopping:
            tooltip = i18n("LiteLLM Proxy: Stopping...");
            break;
        case LiteLLMProcess::Error:
            tooltip = i18n("LiteLLM Proxy: Error");
            break;
        default:
            tooltip = i18n("LiteLLM Proxy: Stopped");
            break;
    }
    TQToolTip::add(this, tooltip);
}