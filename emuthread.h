#ifndef EMUTHREAD_H
#define EMUTHREAD_H

#include <QThread>
#include <QTimer>

class EmuThread : public QThread
{
    Q_OBJECT
public:
    explicit EmuThread(QObject *parent = 0);

    void doStuff();
    void throttleTimerWait();

    volatile bool paused = false;

    std::string boot1 = "", flash = "";
    unsigned int port_gdb = 0, port_rdbg = 0;

signals:
    void exited(int retcode);
    void serialChar(char c);
    void debugStr(QString str);
    void speedChanged(double value);
    void statusMsg(QString str);
    void usblinkChanged(bool state);
    void throttleTimerChanged(bool state);

public slots:
    virtual void run() override;
    void setThrottleTimer(bool state);
    void setThrottleTimerDeactivated(bool state);
    void enterDebugger();
    void setPaused(bool paused);
    bool stop();
    void reset();

private:
    QTimer throttle_timer;
    bool throttle_timer_state = false;
    bool enter_debugger = false;
};

extern EmuThread *emu_thread;

#endif // EMUTHREAD_H
