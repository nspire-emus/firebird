#ifndef EMUTHREAD_H
#define EMUTHREAD_H

#include <QThread>

class EmuThread : public QThread
{
    Q_OBJECT
public:
    explicit EmuThread(QObject *parent = 0);

signals:
    void exited(int retcode);

public slots:
    virtual void run() override;

};

#endif // EMUTHREAD_H
