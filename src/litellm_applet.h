/*
 * LiteLLM TDE Panel Applet
 * Copyright (c) 2024 Peter Kokosharov <peter.kokosharov@gmail.com>
 * LLM: NVIDIA Nemotron 3 Ultra <noreply@nvidia.com>
 * Licensed under MIT License
 */

#ifndef LITELLM_APPLET_H
#define LITELLM_APPLET_H

#include <kpanelapplet.h>
#include <tdemacros.h>
#include <tqwidget.h>
#include <tqiconset.h>
#include <tqpopupmenu.h>
#include <tqlabel.h>
#include <tqtimer.h>
#include "litellm_process.h"

class TDE_EXPORT LiteLLMApplet : public KPanelApplet {
    TQ_OBJECT

public:
    LiteLLMApplet(const TQString& configFile, TQWidget* parent);
    ~LiteLLMApplet();

    // KPanelApplet virtuals
    int widthForHeight(int h);
    int heightForWidth(int w);
    bool hasConfiguration() { return true; }
    void configure();

private slots:
    void onStart();
    void onStop();
    void onRestart();
    void onShowLogs();
    void onProcessStateChanged(LiteLLMProcess::State state);
    void onProcessOutput(const TQString& line);
    void onProcessError(const TQString& msg);
    void updateIcon();
    void updateTooltip();

private:
    void createMenu();
    void setupConfig();
    TQString configFilePath() const;

    LiteLLMProcess* m_process;
    TQPopupMenu* m_menu;
    TQTimer* m_tooltipTimer;

    // Icons for different states (TQt3 uses TQIconSet)
    TQIconSet m_iconIdle;
    TQIconSet m_iconRunning;
    TQIconSet m_iconError;
    TQIconSet m_iconStopped;
    TQIconSet m_iconStarting;

    // UI
    TQLabel* m_iconLabel;

    // Config
    TQString m_configPath;
    TQString m_binaryPath;
    int m_port;
};

#endif // LITELLM_APPLET_H