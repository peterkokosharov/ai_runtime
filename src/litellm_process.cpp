#include "litellm_process.h"
#include <kdebug.h>
#include <tqtimer.h>

LiteLLMProcess::LiteLLMProcess(TQObject* parent)
    : TQObject(parent)
    , m_process(new TQProcess(this))
    , m_state(Stopped)
    , m_port(4000)
    , m_binary("litellm")
{
    connect(m_process, TQ_SIGNAL(readyReadStdout()), this, TQ_SLOT(onReadyReadStdout()));
    connect(m_process, TQ_SIGNAL(readyReadStderr()), this, TQ_SLOT(onReadyReadStderr()));
    connect(m_process, TQ_SIGNAL(processExited()), this, TQ_SLOT(onProcessExited()));
    connect(m_process, TQ_SIGNAL(processError(int)), this, TQ_SLOT(onProcessError(int)));
}

LiteLLMProcess::~LiteLLMProcess() {
    if (m_state != Stopped) {
        stop();
    }
}

bool LiteLLMProcess::start() {
    if (m_state != Stopped) {
        return false;
    }

    setState(Starting);

    TQStringList args = buildArguments();
    kdDebug() << "Starting LiteLLM:" << m_binary << args << endl;

    m_process->clearArguments();
    m_process->addArgument(m_binary);
    for (TQStringList::ConstIterator it = args.begin(); it != args.end(); ++it) {
        m_process->addArgument(*it);
    }

    if (!m_process->start()) {
        setState(Error);
        emit errorOccurred("Failed to start LiteLLM process");
        return false;
    }

    setState(Running);
    return true;
}

void LiteLLMProcess::stop() {
    if (m_state == Stopped || m_state == Stopping) {
        return;
    }

    setState(Stopping);
    m_process->tryTerminate();

    // Give it a moment to stop
    TQTimer::singleShot(2000, this, TQ_SLOT(kill()));
}

void LiteLLMProcess::kill() {
    if (m_state == Stopped) return;
    m_process->kill();
    setState(Stopped);
}

TQStringList LiteLLMProcess::buildArguments() const {
    TQStringList args;
    args << "--port" << TQString::number(m_port);
    if (!m_configPath.isEmpty()) {
        args << "--config" << m_configPath;
    }
    return args;
}

void LiteLLMProcess::setState(State newState) {
    if (m_state != newState) {
        m_state = newState;
        emit stateChanged(newState);
    }
}

void LiteLLMProcess::onReadyReadStdout() {
    while (m_process->canReadLineStdout()) {
        TQString line = m_process->readLineStdout();
        emit outputReceived(line.stripWhiteSpace());
    }
}

void LiteLLMProcess::onReadyReadStderr() {
    while (m_process->canReadLineStderr()) {
        TQString line = m_process->readLineStderr();
        emit outputReceived("[stderr] " + line.stripWhiteSpace());
    }
}

void LiteLLMProcess::onProcessExited() {
    kdDebug() << "LiteLLM process exited" << endl;
    if (m_state != Stopped && m_state != Stopping) {
        setState(Error);
        emit errorOccurred("Process exited unexpectedly");
    } else {
        setState(Stopped);
    }
}

void LiteLLMProcess::onProcessError(int error) {
    TQString msg;
    switch (error) {
        case 1:  // FailedToStart
            msg = "Failed to start: binary not found or permissions";
            break;
        case 2:  // Crashed
            msg = "Process crashed";
            break;
        case 3:  // Timedout
            msg = "Process timed out";
            break;
        case 4:  // WriteError
            msg = "Write error";
            break;
        case 5:  // ReadError
            msg = "Read error";
            break;
        default:
            msg = "Unknown process error";
    }
    kdDebug() << "LiteLLM process error:" << msg << " (code:" << error << ")" << endl;
    if (m_state != Stopped && m_state != Stopping) {
        setState(Error);
        emit errorOccurred(msg);
    }
}