#ifndef QMLBRIDGE_H
#define QMLBRIDGE_H

#include <QObject>
#include <QtQml>

#include "emuthread.h"

struct Kit
{
    unsigned int id;
    QString name, type, boot1, flash, snapshot;
};

class KitModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Role {
        IDRole = Qt::UserRole + 1,
        NameRole,
        TypeRole,
        FlashRole,
        Boot1Role,
        SnapshotRole
    };
    Q_ENUMS(Role)

    KitModel() = default;
    KitModel(const KitModel &other) : QAbstractListModel() { kits = other.kits; nextID = other.nextID; }
    KitModel &operator =(const KitModel &other) { kits = other.kits; return *this; }

    Q_INVOKABLE virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    Q_INVOKABLE virtual QHash<int, QByteArray> roleNames() const override;
    Q_INVOKABLE virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Q_INVOKABLE virtual QVariant getData(const int row, int role = Qt::DisplayRole) const;
    Q_INVOKABLE bool setData(const int row, const QVariant &value, int role = Qt::EditRole);
    Q_INVOKABLE bool copy(const int row);
    Q_INVOKABLE bool remove(const int row);
    Q_INVOKABLE void addKit(QString name, QString boot1, QString flash, QString snapshot_path);
    Q_INVOKABLE int indexForID(const unsigned int id);

    QString typeForFlash(QString flash);

    /* Doesn't work as class members */
    friend QDataStream &operator<<(QDataStream &out, const KitModel &kits);
    friend QDataStream &operator>>(QDataStream &in, KitModel &kits);

    const QList<Kit> &getKits() const { return kits; }

signals:
    void anythingChanged();

private:
    unsigned int nextID = 0;
    QList<Kit> kits;
};

Q_DECLARE_METATYPE(KitModel)

class QMLBridge : public QObject
{
    Q_OBJECT
public:
    explicit QMLBridge(QObject *parent = 0);
    ~QMLBridge();

    Q_PROPERTY(unsigned int gdbPort READ getGDBPort WRITE setGDBPort NOTIFY gdbPortChanged)
    Q_PROPERTY(bool gdbEnabled READ getGDBEnabled WRITE setGDBEnabled NOTIFY gdbEnabledChanged)
    Q_PROPERTY(unsigned int rdbPort READ getRDBPort WRITE setRDBPort NOTIFY rdbPortChanged)
    Q_PROPERTY(bool rdbEnabled READ getRDBEnabled WRITE setRDBEnabled NOTIFY rdbEnabledChanged)
    Q_PROPERTY(bool debugOnStart READ getDebugOnStart WRITE setDebugOnStart NOTIFY debugOnStartChanged)
    Q_PROPERTY(bool debugOnWarn READ getDebugOnWarn WRITE setDebugOnWarn NOTIFY debugOnWarnChanged)
    Q_PROPERTY(bool autostart READ getAutostart WRITE setAutostart NOTIFY autostartChanged)
    Q_PROPERTY(unsigned int defaultKit READ getDefaultKit WRITE setDefaultKit NOTIFY defaultKitChanged)
    Q_PROPERTY(bool suspendOnClose READ getSuspendOnClose WRITE setSuspendOnClose NOTIFY suspendOnCloseChanged)
    Q_PROPERTY(KitModel* kits READ getKitModel)

    unsigned int getGDBPort();
    void setGDBPort(unsigned int port);
    void setGDBEnabled(bool e);
    bool getGDBEnabled();
    unsigned int getRDBPort();
    void setRDBPort(unsigned int port);
    void setRDBEnabled(bool e);
    bool getRDBEnabled();
    void setDebugOnWarn(bool e);
    bool getDebugOnWarn();
    void setDebugOnStart(bool e);
    bool getDebugOnStart();
    void setAutostart(bool e);
    bool getAutostart();
    unsigned int getDefaultKit();
    void setDefaultKit(unsigned int id);
    bool getSuspendOnClose();
    void setSuspendOnClose(bool e);

    KitModel *getKitModel() { return &kit_model; }
    Q_INVOKABLE void keypadStateChanged(int keymap_id, bool state);
    Q_INVOKABLE void registerNButton(int keymap_id, QVariant button);

    // Coordinates: (0/0) = top left (1/1) = bottom right
    Q_INVOKABLE void touchpadStateChanged(qreal x, qreal y, bool state);
    Q_INVOKABLE void registerTouchpad(QVariant touchpad);

    Q_INVOKABLE bool isMobile();

    // Various utility functions
    Q_INVOKABLE QString basename(QString path);
    Q_INVOKABLE QString dir(QString path);
    Q_INVOKABLE QString toLocalFile(QUrl url);
    Q_INVOKABLE int kitIndexForID(unsigned int id);

    #ifdef MOBILE_UI
        Q_INVOKABLE bool restart();
        Q_INVOKABLE void setPaused(bool b);
        Q_INVOKABLE void reset();
        Q_INVOKABLE void suspend();
        Q_INVOKABLE void resume();
        Q_INVOKABLE bool stop();

        Q_INVOKABLE bool saveFlash();

        Q_INVOKABLE QString getBoot1Path();
        Q_INVOKABLE void setBoot1Path(QUrl path);
        Q_INVOKABLE QString getFlashPath();
        Q_INVOKABLE void setFlashPath(QUrl path);
        Q_INVOKABLE QString getSnapshotPath();
        Q_INVOKABLE void setSnapshotPath(QUrl path);

        Q_INVOKABLE void registerToast(QVariant toast);
        Q_INVOKABLE void toastMessage(QString msg);

        EmuThread emu_thread;
    #endif

public slots:
    void saveKits();
    #ifdef MOBILE_UI
        void started(bool success); // Not called on resume
        void resumed(bool success);
        void suspended(bool success);

    #endif

signals:
    void gdbPortChanged();
    void gdbEnabledChanged();
    void rdbPortChanged();
    void rdbEnabledChanged();
    void debugOnWarnChanged();
    void debugOnStartChanged();
    void autostartChanged();
    void defaultKitChanged();
    void suspendOnCloseChanged();

private:
    QObject *toast = nullptr;
    KitModel kit_model;
    QSettings settings;
};

extern QMLBridge *the_qml_bridge;

void notifyKeypadStateChanged(int row, int col, bool state);
void notifyTouchpadStateChanged();
void notifyTouchpadStateChanged(qreal x, qreal y, bool state);
QObject *qmlBridgeFactory(QQmlEngine *engine, QJSEngine *scriptEngine);


#endif // QMLBRIDGE_H
