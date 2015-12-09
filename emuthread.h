#ifndef EMUTHREAD_H
#define EMUTHREAD_H

#include <QThread>

class EmuThread : public QThread
{
    Q_OBJECT
public:
    explicit EmuThread(QObject *parent = 0);

    void doStuff(bool wait);
    void throttleTimerWait();

    QString boot1, flash;
    unsigned int port_gdb = 0, port_rdbg = 0;

signals:
    // State
    void started(bool success); // Not called on resume
    void resumed(bool success);
    void suspended(bool success);
    void stopped();
    void paused(bool b);

    // I/O
    void serialChar(char c);

    // Status
    void isBusy(bool busy);
    void statusMsg(QString str);
    void speedChanged(double value);
    void usblinkChanged(bool state);
    void turboModeChanged(bool state);

    // Debugging
    void debugStr(QString str);
    void debuggerEntered(bool state);
    void debugInputRequested(bool b);

public slots:
    virtual void run() override;

    // State
    void setPaused(bool is_paused);
    bool stop();
    void reset();
    bool resume(QString path);
    void suspend(QString path);

    // Emulation settings
    void setTurboMode(bool state);
    void toggleTurbo();

    // Debugging
    void enterDebugger();
    void debuggerInput(QString str);

private:
    bool enter_debugger = false;
    volatile bool is_paused = false, do_suspend = false, do_resume = false;
    std::string debug_input, snapshot_path;
};

extern EmuThread *emu_thread;

#endif // EMUTHREAD_H
