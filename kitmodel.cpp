#include "core/flash.h"
#include "core/os/os.h"

#include "kitmodel.h"

QDataStream &operator<<(QDataStream &out, const Kit &kit)
{
    unsigned int version = 1;

    out << version
        << kit.id
        << kit.name
        << kit.boot1
        << kit.flash
        << kit.snapshot
        << kit.type;

    return out;
}

QDataStream &operator>>(QDataStream &in, Kit &kit)
{
    unsigned int version;
    in >> version;

    if(version == 1)
    {
        in >> kit.id
           >> kit.name
           >> kit.boot1
           >> kit.flash
           >> kit.snapshot
           >> kit.type;
    }
    else
        qWarning() << "Unknown Kit serialization version " << version;

    return in;
}

QDataStream &operator<<(QDataStream &out, const KitModel &kits)
{
    unsigned int version = 1;

    out << version << kits.kits << kits.nextID;
    return out;
}

QDataStream &operator>>(QDataStream &in, KitModel &kits)
{
    unsigned int version;
    in >> version;

    if(version == 1)
        in >> kits.kits >> kits.nextID;
    else
        qWarning() << "Unknown KitModel serialization version " << version;

    return in;
}

int KitModel::rowCount(const QModelIndex &) const
{
    return kits.count();
}

QHash<int, QByteArray> KitModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[IDRole] = "id";
    roles[NameRole] = "name";
    roles[TypeRole] = "type";
    roles[FlashRole] = "flash";
    roles[Boot1Role] = "boot1";
    roles[SnapshotRole] = "snapshot";
    return roles;
}

QVariant KitModel::data(const QModelIndex &index, int role) const
{
    return getDataRow(index.row(), role);
}

QVariant KitModel::getDataRow(const int row, int role) const
{
    if(row < 0 || row >= kits.count())
        return QVariant();

    switch(role)
    {
    case IDRole:
        return kits[row].id;
    case NameRole:
        return kits[row].name;
    case TypeRole:
        return kits[row].type;
    case FlashRole:
        return kits[row].flash;
    case Boot1Role:
        return kits[row].boot1;
    case SnapshotRole:
        return kits[row].snapshot;
    default:
        return QVariant();
    }
}

bool KitModel::setDataRow(const int row, const QVariant &value, int role)
{
    if(row < 0 || row >= kits.count())
        return false;

    switch(role)
    {
    case IDRole:
        qWarning("ID not assignable");
        return false;
        break;
    case NameRole:
        kits[row].name = value.toString();
        break;
    case TypeRole:
        kits[row].type = value.toString();
        break;
    case FlashRole:
        kits[row].flash = value.toString();
        break;
    case Boot1Role:
        kits[row].boot1 = value.toString();
        break;
    case SnapshotRole:
        kits[row].snapshot = value.toString();
        break;
    default:
        return false;
    }

    if(role == FlashRole)
    {
        // Refresh type as well
        kits[row].type = typeForFlash(value.toString());
        emit dataChanged(index(row), index(row), QVector<int>({FlashRole, TypeRole}));
    }
    else
        emit dataChanged(index(row), index(row), QVector<int>({role}));

    emit anythingChanged();
    return true;
}

bool KitModel::copy(const int row)
{
    if(row < 0 || row >= kits.count())
        return false;

    beginInsertRows({}, row, row);
    kits.insert(row, kits[row]);
    kits[row].id = nextID++;
    endInsertRows();

    emit anythingChanged();

    return true;
}

bool KitModel::remove(const int row)
{
    if(row < 0 || row >= kits.count())
        return false;

    // Do not remove the last remaining kit
    if(kits.count() == 1)
        return false;

    beginRemoveRows({}, row, row);
    kits.removeAt(row);
    endRemoveRows();

    emit anythingChanged();

    return true;
}

void KitModel::addKit(QString name, QString boot1, QString flash, QString snapshot_path)
{
    int row = kits.size();
    beginInsertRows({}, row, row);
    kits.append({nextID++, name, typeForFlash(flash), boot1, flash, snapshot_path});
    endInsertRows();

    emit anythingChanged();
}

int KitModel::indexForID(const unsigned int id)
{
    auto it = std::find_if(kits.begin(), kits.end(),
                           [&] (const Kit &kit) { return kit.id == id; });
    return it == kits.end() ? -1 : (it - kits.begin());
}

QString KitModel::typeForFlash(QString flash)
{
    QString type = QStringLiteral("???");
    QByteArray flash_path = flash.toUtf8();
    FILE *file = fopen_utf8(flash_path.data(), "rb");
    if(file)
    {
        std::string s = flash_read_type(file);
        if(!s.empty())
            type = QString::fromStdString(s);
        fclose(file);
    }

    return type;
}
