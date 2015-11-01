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

    volatile bool paused = false, do_suspend = false, do_resume = false;

    std::string boot1 = "", flash = "", snapshot_path = "";
    unsigned int port_gdb = 0, port_rdbg = 0;

signals:
    void exited(int retcode);
    void serialChar(char c);
    void debugStr(QString str);
    void speedChanged(double value);
    void statusMsg(QString str);
    void usblinkChanged(bool state);
    void turboModeChanged(bool state);
    void debuggerEntered(bool state);

public slots:
    virtual void run() override;
    void setTurboMode(bool state);
    void toggleTurbo();
    void enterDebugger();
    void setPaused(bool paused);
    bool stop();
    void reset();

private:
    bool enter_debugger = false;
};

extern EmuThread *emu_thread;

#endif // EMUTHREAD_H
