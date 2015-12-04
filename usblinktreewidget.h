#ifndef USBLINKTREEWIDGET_H
#define USBLINKTREEWIDGET_H

#include <atomic>

#include <QTreeWidget>

class USBLinkTreeWidget : public QTreeWidget
{
    Q_OBJECT

public:
    USBLinkTreeWidget(QWidget *parent = 0);

    // usblink callbacks
    static void usblink_dirlist_callback_nested(struct usblink_file *file, bool is_error, void *data);
    static void usblink_dirlist_callback(struct usblink_file *file, bool is_error, void *data);
    static void usblink_delete_callback(int progress, void *data);
    static void usblink_upload_callback(int progress, void *data);
    static void usblink_download_callback(int progress, void *data);
    static void usblink_move_progress(int progress, void *user_data);

    // Helper functions for usblink callbacks
    static bool usblink_dirlist_nested(QTreeWidgetItem *w);
    static QString usblink_path_item(QTreeWidgetItem *w);

protected:
    virtual QStringList mimeTypes() const override;
    virtual void dragEnterEvent(QDragEnterEvent *e) override;
    virtual bool dropMimeData(QTreeWidgetItem *parent, int index, const QMimeData *data, Qt::DropAction action) override;

signals:
    void downloadProgress(int progress); // Called by callback from EmuThread. Use Qt::QueuedConnection
    void uploadProgress(int progress); // Same as above

    void wantToAddTreeItem(QTreeWidgetItem *item, QTreeWidgetItem *parent); // To let it run in the UI thread. Internal
    void wantToReload(); // Same as above

public slots:
    void reloadFilebrowser();
    void customContextMenuRequested(QPoint pos); // Internal
    void dataChanged(QTreeWidgetItem *item, int column); // Internal
    void downloadEntry(); // Internal
    void deleteEntry(); // Internal
    void addTreeItem(QTreeWidgetItem *item, QTreeWidgetItem *parent); // Has to run in the UI thread. Internal

private:
    // To avoid concurrent dirlisting
    std::atomic<bool> doing_dirlist{false};
};

#endif // USBLINKTREEWIDGET_H
