#ifndef LITELLM_PROCESS_H
#define LITELLM_PROCESS_H

#include <tqobject.h>
#include <tqprocess.h>
#include <tqstring.h>
#include <tqstringlist.h>

class LiteLLMProcess : public TQObject {
    TQ_OBJECT

public:
    enum State {
        Stopped,
        Starting,
        Running,
        Stopping,
        Error
    };
    TQ_ENUMS(State)

    explicit LiteLLMProcess(TQObject* parent = nullptr);
    ~LiteLLMProcess() override;

    State state() const { return m_state; }
    TQString configPath() const { return m_configPath; }
    void setConfigPath(const TQString& path) { m_configPath = path; }

    int port() const { return m_port; }
    void setPort(int port) { m_port = port; }

    TQString liteLLMBinary() const { return m_binary; }
    void setLiteLLMBinary(const TQString& binary) { m_binary = binary; }

    // Start the process
    bool start();
    // Stop the process gracefully
    void stop();
    // Force kill
    void kill();

signals:
    void stateChanged(State newState);
    void outputReceived(const TQString& line);
    void errorOccurred(const TQString& msg);

private slots:
    void onReadyReadStdout();
    void onReadyReadStderr();
    void onProcessExited();
    void onProcessError(int error);

private:
    void setState(State newState);
    TQStringList buildArguments() const;

    TQProcess* m_process;
    State m_state;
    TQString m_configPath;
    int m_port;
    TQString m_binary;
};

#endif // LITELLM_PROCESS_H
