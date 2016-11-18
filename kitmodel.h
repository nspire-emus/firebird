#ifndef KITMODEL_H
#define KITMODEL_H

#include <QtQml>

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
    KitModel &operator =(const KitModel &other) { kits = other.kits; nextID = other.nextID; return *this; }

    Q_INVOKABLE virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    Q_INVOKABLE virtual QHash<int, QByteArray> roleNames() const override;
    Q_INVOKABLE virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Q_INVOKABLE QVariant getDataRow(const int row, int role = Qt::DisplayRole) const;
    Q_INVOKABLE bool setDataRow(const int row, const QVariant &value, int role = Qt::EditRole);
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


#endif // KITMODEL_H
