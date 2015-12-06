#include <QDragEnterEvent>
#include <QFileDialog>
#include <QMenu>
#include <QMimeData>

#include "usblinktreewidget.h"

#include "core/usblink_queue.h"

static USBLinkTreeWidget *usblink_tree = nullptr;

USBLinkTreeWidget::USBLinkTreeWidget(QWidget *parent)
    : QTreeWidget(parent)
{
    connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(customContextMenuRequested(QPoint)));
    connect(this, SIGNAL(itemChanged(QTreeWidgetItem*,int)), this, SLOT(dataChangedHandler(QTreeWidgetItem*,int)));
    // This is a Qt::BlockingQueuedConnection as the usblink_dirlist_* family of functions needs to enumerate over the items directly after emitting the signal.
    connect(this, SIGNAL(wantToAddTreeItem(QTreeWidgetItem*,QTreeWidgetItem*)), this, SLOT(addTreeItem(QTreeWidgetItem*,QTreeWidgetItem*)), Qt::BlockingQueuedConnection);
    connect(this, SIGNAL(wantToReload()), this, SLOT(reloadFilebrowser()), Qt::QueuedConnection);

    this->setAcceptDrops(true);

    usblink_tree = this;
}

void USBLinkTreeWidget::usblink_delete_callback(int progress, void *data)
{
    // Only remove the treewidget item if the delete operation was successful
    if(progress != 100)
        return;

    QTreeWidgetItem *w = static_cast<QTreeWidgetItem*>(data);
    if(w->parent())
        w->parent()->takeChild(w->parent()->indexOfChild(w));
    else
        w->treeWidget()->takeTopLevelItem(w->treeWidget()->indexOfTopLevelItem(w));
}

void USBLinkTreeWidget::usblink_upload_callback(int progress, void *data)
{
    USBLinkTreeWidget *that = static_cast<USBLinkTreeWidget*>(data);

    // TODO: Don't do a full refresh
    if(progress == 100)
        emit that->wantToReload();

    emit that->uploadProgress(progress);
}

void USBLinkTreeWidget::usblink_download_callback(int progress, void *data)
{
    USBLinkTreeWidget *that = static_cast<USBLinkTreeWidget*>(data);
    emit that->downloadProgress(progress);
}

void USBLinkTreeWidget::usblink_move_progress(int progress, void *user_data)
{
    auto *item = reinterpret_cast<QTreeWidgetItem*>(user_data);

    if(progress == 100) // Success
        item->setData(2, Qt::UserRole, item->data(0, Qt::DisplayRole)); // Set internal name to new name
    else if(progress < 0) // Failure
        item->setData(0, Qt::DisplayRole, item->data(2, Qt::UserRole)); // Reset display name to old name
}

void USBLinkTreeWidget::reloadFilebrowser()
{
    bool is_false = false;
    if(!doing_dirlist.compare_exchange_strong(is_false, true))
        usblink_queue_reset(); // The treeWidget is cleared, so references to items get invalid -> Get rid of them!

    if(!usblink_connected)
        usblink_connect();

    this->clear();
    usblink_queue_dirlist("/", usblink_dirlist_callback, this);
}

void USBLinkTreeWidget::customContextMenuRequested(QPoint pos)
{
    if(!this->currentItem())
        return;

    QMenu *menu = new QMenu(this);
    QAction *action_delete = new QAction(tr("Delete"), menu);

    if(this->currentItem()->data(0, Qt::UserRole).toBool() == false)
    {
        // Is not a directory
        QAction *action_download = new QAction(tr("Download"), menu);
        connect(action_download, SIGNAL(triggered()), this, SLOT(downloadEntry()));
        menu->addAction(action_download);
    }
    else if(this->currentItem()->childCount() > 0)
    {
        // Non-empty directory
        action_delete->setDisabled(true);
    }

    connect(action_delete, SIGNAL(triggered()), this, SLOT(deleteEntry()));
    menu->addAction(action_delete);

    menu->popup(this->viewport()->mapToGlobal(pos));
}

static QString naturalSize(uint64_t bytes)
{
    if(bytes < 4ul * 1024)
        return QString::number(bytes) + " B";
    else if(bytes < 4ul * 1024 * 1024)
        return QString::number(bytes / 1024) + " KiB";
    else if(bytes < 4ull * 1024 * 1024 * 1024)
        return QString::number(bytes / 1024 / 1024) + " MiB";
    else
        return QString("Too much");
}

QString USBLinkTreeWidget::usblink_path_item(QTreeWidgetItem *w)
{
    if(!w)
        return "";

    return usblink_path_item(w->parent()) + "/" + w->text(0);
    // This crashes on 32-bit linux somehow
    //return QString("%0/%1").arg(path_parent).arg(path_this);
}

QStringList USBLinkTreeWidget::mimeTypes() const
{
    // Accept everything here, decide based on the filename in dragEnterEvent
    return QStringList("text/uri-list");
}

void USBLinkTreeWidget::dragEnterEvent(QDragEnterEvent *e)
{
    // Somehow caching this QList is necessary. Without this, the values vanished in the middle of the if() condition...
    QList<QUrl> urls = e->mimeData()->urls();
    if(urls.size() == 1 && urls[0].fileName().endsWith(".tns"))
        QTreeWidget::dragEnterEvent(e);
    else
        e->ignore();
}

bool USBLinkTreeWidget::dropMimeData(QTreeWidgetItem *parent, int index, const QMimeData *data, Qt::DropAction action)
{
    if(data->urls().size() != 1)
        return false;

    (void) index;
    (void) action;

    std::string file = QDir::toNativeSeparators(data->urls()[0].toLocalFile()).toStdString();
    usblink_queue_put_file(file, usblink_path_item(parent).toStdString(), usblink_upload_callback, this);
    return true;
}

bool USBLinkTreeWidget::usblink_dirlist_nested(QTreeWidgetItem *w)
{
    // Find a directory (w or its children) to fill

    if(w->data(0, Qt::UserRole).value<bool>() == false) //Not a directory
        return false;

    if(w->data(1, Qt::UserRole).value<bool>() == false) //Not filled yet
    {
        std::string path_utf8 = usblink_path_item(w).toStdString();
        usblink_queue_dirlist(path_utf8, USBLinkTreeWidget::usblink_dirlist_callback_nested, w);
        return true;
    }
    else
    {
        for(int i = 0; i < w->childCount(); ++i)
            if(usblink_dirlist_nested(w->child(i)))
                return true;
    }

    return false;
}

static QTreeWidgetItem *itemForUSBLinkFile(struct usblink_file *file)
{
    QString filename = QString::fromUtf8(file->filename);
    QTreeWidgetItem *item = new QTreeWidgetItem({filename, file->is_dir ? "" : naturalSize(file->size)});
    if(file->is_dir)
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsDropEnabled | Qt::ItemIsEditable | Qt::ItemIsEnabled);
    else
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled);
    item->setData(0, Qt::UserRole, QVariant(file->is_dir));
    item->setData(1, Qt::UserRole, QVariant(false));
    item->setData(2, Qt::UserRole, QVariant(filename));
    return item;
}

void USBLinkTreeWidget::usblink_dirlist_callback_nested(struct usblink_file *file, bool is_error, void *data)
{
    QTreeWidgetItem *w = static_cast<QTreeWidgetItem*>(data);

    //End of enumeration or error
    if(!file)
    {
        w->setData(1, Qt::UserRole, QVariant(true)); //Dir is now filled

        if(!is_error)
        {
            //Find a dir to fill with entries
            for(int i = 0; i < w->treeWidget()->topLevelItemCount(); ++i)
                if(usblink_dirlist_nested(w->treeWidget()->topLevelItem(i)))
                    return;
        }

        // FIXME: If a file is transferred concurrently, this may never be set to false.
        if(usblink_queue_size() == 1)
            usblink_tree->doing_dirlist = false;

        return;
    }

    //Add directory entry to tree widget item (parent)
    emit usblink_tree->wantToAddTreeItem(itemForUSBLinkFile(file), w);
}

void USBLinkTreeWidget::usblink_dirlist_callback(struct usblink_file *file, bool is_error, void *data)
{
    if(is_error)
        return;

    QTreeWidget *w = static_cast<QTreeWidget*>(data);

    //End of enumeration or error
    if(!file)
    {
        //Find a dir to fill with entries
        for(int i = 0; i < w->topLevelItemCount(); ++i)
            if(usblink_dirlist_nested(w->topLevelItem(i)))
                return;

        // FIXME: If a file is transferred concurrently, this may never be set to false.
        if(usblink_queue_size() == 1)
            usblink_tree->doing_dirlist = false;

        return;
    }

    //Add directory entry to tree widget
    emit usblink_tree->wantToAddTreeItem(itemForUSBLinkFile(file), nullptr);
}

void USBLinkTreeWidget::dataChangedHandler(QTreeWidgetItem *item, int column)
{
    // Only the name can be changed
    if(column != 0)
        return;

    std::string filepath = usblink_path_item(item->parent()).toStdString();

    std::string old_name = item->data(2, Qt::UserRole).toString().toStdString(),
             new_name = item->data(0, Qt::DisplayRole).toString().toStdString();

    usblink_queue_move(filepath + "/" + old_name, filepath + "/" + new_name, usblink_move_progress, item);
}

void USBLinkTreeWidget::downloadEntry()
{
    if(!this->currentItem()
            || this->currentItem()->data(0, Qt::UserRole).toBool()) // Is a directory
        return;

    QString dest = QFileDialog::getSaveFileName(this, tr("Chose save location"), QString(), tr("TNS file (*.tns)"));
    if(!dest.isEmpty())
        usblink_queue_download(usblink_path_item(this->currentItem()).toStdString(), dest.toStdString(), usblink_download_callback, this);
}

void USBLinkTreeWidget::deleteEntry()
{
    if(!this->currentItem())
        return;

    usblink_queue_delete(usblink_path_item(this->currentItem()).toStdString(), this->currentItem()->data(0, Qt::UserRole).toBool(), usblink_delete_callback, this->currentItem());
}

void USBLinkTreeWidget::addTreeItem(QTreeWidgetItem *item, QTreeWidgetItem *parent)
{
    if(parent)
        parent->addChild(item);
    else
        this->addTopLevelItem(item);
}
