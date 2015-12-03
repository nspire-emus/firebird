#include <future>

#include <QFileDialog>
#include <QStandardPaths>
#include <QTextBlock>
#include <QMessageBox>
#include <QGraphicsItem>
#include <QDropEvent>
#include <QMimeData>
#include <QDockWidget>
#include <QShortcut>

#include "core/debug.h"
#include "core/emu.h"
#include "core/flash.h"
#include "core/gif.h"
#include "core/misc.h"
#include "core/usblink_queue.h"

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "qmlbridge.h"
#include "qtframebuffer.h"
#include "qtkeypadbridge.h"

MainWindow *main_window;
// Change this if you change the UI
static const constexpr int WindowStateVersion = 0;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->statusBar->addWidget(&status_label);

    // Register QtKeypadBridge for the virtual keyboard functionality
    ui->keypadWidget->installEventFilter(&qt_keypad_bridge);
    ui->lcdView->installEventFilter(&qt_keypad_bridge);

    //Emu -> GUI (QueuedConnection as they're different threads)
    connect(&emu, SIGNAL(serialChar(char)), this, SLOT(serialChar(char)), Qt::QueuedConnection);
    connect(&emu, SIGNAL(debugStr(QString)), this, SLOT(debugStr(QString))); //Not queued connection as it may cause a hang
    connect(&emu, SIGNAL(speedChanged(double)), this, SLOT(showSpeed(double)), Qt::QueuedConnection);
    connect(&emu, SIGNAL(statusMsg(QString)), this, SLOT(showStatusMsg(QString)), Qt::QueuedConnection);
    connect(&emu, SIGNAL(turboModeChanged(bool)), ui->buttonSpeed, SLOT(setChecked(bool)), Qt::QueuedConnection);
    connect(&emu, SIGNAL(usblinkChanged(bool)), this, SLOT(usblinkChanged(bool)), Qt::QueuedConnection);
    connect(&emu, SIGNAL(debugInputRequested(bool)), this, SLOT(debugInputRequested(bool)), Qt::QueuedConnection);
    connect(&emu, SIGNAL(started(bool)), this, SLOT(started(bool)), Qt::QueuedConnection);
    connect(&emu, SIGNAL(paused(bool)), ui->actionPause, SLOT(setChecked(bool)), Qt::QueuedConnection);
    connect(&emu, SIGNAL(resumed(bool)), this, SLOT(resumed(bool)), Qt::QueuedConnection);
    connect(&emu, SIGNAL(suspended(bool)), this, SLOT(suspended(bool)), Qt::QueuedConnection);
    connect(&emu, SIGNAL(stopped()), this, SLOT(stopped()), Qt::QueuedConnection);

    //GUI -> Emu (no QueuedConnection possible, watch out!)
    connect(this, SIGNAL(debuggerCommand(QString)), &emu, SLOT(debuggerInput(QString)));

    //Menu "Emulator"
    connect(ui->buttonReset, SIGNAL(clicked(bool)), &emu, SLOT(reset()));
    connect(ui->actionReset, SIGNAL(triggered()), &emu, SLOT(reset()));
    connect(ui->actionRestart, SIGNAL(triggered()), this, SLOT(restart()));
    connect(ui->actionDebugger, SIGNAL(triggered()), &emu, SLOT(enterDebugger()));
    connect(ui->buttonPause, SIGNAL(clicked(bool)), &emu, SLOT(setPaused(bool)));
    connect(ui->buttonPause, SIGNAL(clicked(bool)), ui->actionPause, SLOT(setChecked(bool)));
    connect(ui->actionPause, SIGNAL(toggled(bool)), &emu, SLOT(setPaused(bool)));
    connect(ui->actionPause, SIGNAL(toggled(bool)), ui->buttonPause, SLOT(setChecked(bool)));
    connect(ui->buttonSpeed, SIGNAL(clicked(bool)), &emu, SLOT(setTurboMode(bool)));
    QShortcut *shortcut = new QShortcut(QKeySequence(Qt::Key_F11), this);
    shortcut->setAutoRepeat(false);
    connect(shortcut, SIGNAL(activated()), &emu, SLOT(toggleTurbo()));

    //Menu "Tools"
    connect(ui->buttonScreenshot, SIGNAL(clicked()), this, SLOT(screenshot()));
    connect(ui->actionScreenshot, SIGNAL(triggered()), this, SLOT(screenshot()));
    connect(ui->actionRecord_GIF, SIGNAL(triggered()), this, SLOT(recordGIF()));
    connect(ui->actionConnect, SIGNAL(triggered()), this, SLOT(connectUSB()));
    connect(ui->buttonUSB, SIGNAL(clicked(bool)), this, SLOT(connectUSB()));
    connect(ui->actionXModem, SIGNAL(triggered()), this, SLOT(xmodemSend()));
    ui->actionConnect->setShortcut(QKeySequence(Qt::Key_F10));
    ui->actionConnect->setAutoRepeat(false);

    //Menu "State"
    connect(ui->actionResume, SIGNAL(triggered()), this, SLOT(resume()));
    connect(ui->actionSuspend, SIGNAL(triggered()), this, SLOT(suspend()));
    connect(ui->actionResume_from_file, SIGNAL(triggered()), this, SLOT(resumeFromFile()));
    connect(ui->actionSuspend_to_file, SIGNAL(triggered()), this, SLOT(suspendToFile()));

    //Menu "Flash"
    connect(ui->actionSave, SIGNAL(triggered()), this, SLOT(saveFlash()));
    connect(ui->actionCreate_flash, SIGNAL(triggered()), this, SLOT(createFlash()));

    //Menu "About"
    connect(ui->actionAbout_Firebird, SIGNAL(triggered(bool)), this, SLOT(showAbout()));
    connect(ui->actionAbout_Qt, SIGNAL(triggered(bool)), qApp, SLOT(aboutQt()));

    //Debugging
    connect(ui->lineEdit, SIGNAL(returnPressed()), this, SLOT(debugCommand()));

    //File transfer
    connect(ui->refreshButton, SIGNAL(clicked(bool)), this, SLOT(reload_filebrowser()));
    connect(this, SIGNAL(usblink_progress_changed(int)), this, SLOT(changeProgress(int)), Qt::QueuedConnection);
    connect(ui->treeWidget, SIGNAL(itemChanged(QTreeWidgetItem*,int)), this, SLOT(usblink_dirlist_dataChanged(QTreeWidgetItem*,int)));
    connect(ui->treeWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(usblinkContextMenu(QPoint)));
    connect(this, SIGNAL(wantToAddTreeItem(QTreeWidgetItem*,QTreeWidgetItem*)), this, SLOT(addTreeItem(QTreeWidgetItem*,QTreeWidgetItem*)), Qt::QueuedConnection);

    //Settings
    connect(ui->checkDebugger, SIGNAL(toggled(bool)), this, SLOT(setDebuggerOnStartup(bool)));
    connect(ui->checkWarning, SIGNAL(toggled(bool)), this, SLOT(setDebuggerOnWarning(bool)));
    connect(ui->uiDocks, SIGNAL(toggled(bool)), this, SLOT(setUIMode(bool)));
    connect(ui->checkAutostart, SIGNAL(toggled(bool)), this, SLOT(setAutostart(bool)));
    connect(ui->fileBoot1, SIGNAL(pressed()), this, SLOT(selectBoot1()));
    connect(ui->fileFlash, SIGNAL(pressed()), this, SLOT(selectFlash()));
    connect(ui->pathTransfer, SIGNAL(textEdited(QString)), this, SLOT(setUSBPath(QString)));
    connect(ui->spinGDB, SIGNAL(valueChanged(int)), this, SLOT(setGDBPort(int)));
    connect(ui->spinRDBG, SIGNAL(valueChanged(int)), this, SLOT(setRDBGPort(int)));
    connect(ui->orderDiags, SIGNAL(toggled(bool)), this, SLOT(setBootOrder(bool)));
    connect(ui->checkSuspend, SIGNAL(toggled(bool)), this, SLOT(setSuspendOnClose(bool)));
    connect(ui->checkResume, SIGNAL(toggled(bool)), this, SLOT(setResumeOnOpen(bool)));
    connect(ui->buttonChangeSnapshotPath, SIGNAL(clicked()), this, SLOT(changeSnapshotPath()));

    //FlashDialog
    connect(&flash_dialog, SIGNAL(flashCreated(QString)), this, SLOT(flashCreated(QString)));

    //Set up monospace fonts
    QFont monospace = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    ui->debugConsole->setFont(monospace);
    ui->serialConsole->setFont(monospace);

    qRegisterMetaType<QVector<int>>();

#ifdef Q_OS_ANDROID
    //On android the settings file is deleted everytime you update or uninstall,
    //so choose a better, safer, location
    QString path = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    settings = new QSettings(path + "/nspire_emu.ini", QSettings::IniFormat);
#else
    settings = new QSettings();
#endif

    updateUIActionState(false);

    //Load settings
    setUIMode(settings->value("docksEnabled", true).toBool());
    restoreGeometry(settings->value("windowGeometry").toByteArray());
    restoreState(settings->value("windowState").toByteArray(), WindowStateVersion);
    selectBoot1(settings->value("boot1", "").toString());
    selectFlash(settings->value("flash", "").toString());
    setDebuggerOnStartup(settings->value("debugOnStart", false).toBool());
    setDebuggerOnWarning(settings->value("debugOnWarn", false).toBool());
    setUSBPath(settings->value("usbdir", QString("ndless")).toString());
    setGDBPort(settings->value("gdbPort", 3333).toUInt());
    setRDBGPort(settings->value("rdbgPort", 3334).toUInt());
    ui->checkSuspend->setChecked(settings->value("suspendOnClose", false).toBool());
    ui->checkResume->setChecked(settings->value("resumeOnOpen", false).toBool());
    if(!settings->value("snapshotPath").toString().isEmpty())
        ui->labelSnapshotPath->setText(settings->value("snapshotPath").toString());

    setBootOrder(false);

    if(settings->value("resumeOnOpen").toBool())
        resume();
    else
    {
        bool autostart = settings->value("emuAutostart", false).toBool();
        setAutostart(autostart);
        if(emu.boot1 != "" && emu.flash != "" && autostart)
            emu.start();
        else
            showStatusMsg(tr("Start the emulation via Emulation->Restart."));
    }

    ui->lcdView->setFocus();

    #ifdef Q_OS_MAC
        QTimer::singleShot(50, [&] {dockVisibilityChanged(false);}); // Trigger dock update (after UI was shown)
    #endif
}

MainWindow::~MainWindow()
{
    settings->setValue("windowState", saveState(WindowStateVersion));
    settings->setValue("windowGeometry", saveGeometry());

    delete settings;
    delete ui;
}

void MainWindow::dropEvent(QDropEvent *e)
{
    const QMimeData* mime_data = e->mimeData();
    if(!mime_data->hasUrls())
        return;

    if(!usblink_connected)
        usblink_connect();

    for(auto &&url : mime_data->urls())
    {
        QUrl local = url.toLocalFile();
        usblink_queue_put_file(local.toString().toStdString(), settings->value("usbdir", QString("ndless")).toString().toStdString(), usblink_progress_callback, this);
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent *e)
{
    e->accept();
}

void MainWindow::serialChar(const char c)
{
    ui->serialConsole->moveCursor(QTextCursor::End);

    static char previous = 0;

    switch(c)
    {
        case 0:

        case '\r':
            previous = c;
            break;

        case '\b':
            ui->serialConsole->textCursor().deletePreviousChar();
            break;

        default:
            if(c != '\n' && previous == '\r')
            {
                ui->serialConsole->moveCursor(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);
                ui->serialConsole->moveCursor(QTextCursor::End, QTextCursor::KeepAnchor);
                ui->serialConsole->textCursor().removeSelectedText();
                previous = 0;
            }
            ui->serialConsole->insertPlainText(QString(c));
    }
}

void MainWindow::debugInputRequested(bool b)
{
    ui->lineEdit->setEnabled(b);

    if(b)
    {
        raiseDebugger();
        ui->lineEdit->setFocus();
    }
}

void MainWindow::debugStr(QString str)
{
    ui->debugConsole->moveCursor(QTextCursor::End);
    ui->debugConsole->insertPlainText(str);

    raiseDebugger();
}

void MainWindow::debugCommand()
{
    emit debuggerCommand(ui->lineEdit->text());
    ui->lineEdit->clear();
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

static QString usblink_path_item(QTreeWidgetItem *w)
{
    if(!w)
        return "";

    return usblink_path_item(w->parent()) + "/" + w->text(0);
    // This crashes on 32-bit linux somehow
    //return QString("%0/%1").arg(path_parent).arg(path_this);
}

static bool usblink_dirlist_nested(QTreeWidgetItem *w)
{
    if(w->data(0, Qt::UserRole).value<bool>() == false) //Not a directory
        return false;

    if(w->data(1, Qt::UserRole).value<bool>() == false) //Not filled yet
    {
        QByteArray path_utf8 = usblink_path_item(w).toUtf8();
        usblink_queue_dirlist(path_utf8.data(), MainWindow::usblink_dirlist_callback_nested, w);
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

void MainWindow::usblink_dirlist_callback_nested(struct usblink_file *file, void *data)
{
    QTreeWidgetItem *w = static_cast<QTreeWidgetItem*>(data);

    //End of enumeration or error
    if(!file)
    {
        w->setData(1, Qt::UserRole, QVariant(true)); //Dir is now filled
        //Find a dir to fill with entries
        for(int i = 0; i < w->treeWidget()->topLevelItemCount(); ++i)
            if(usblink_dirlist_nested(w->treeWidget()->topLevelItem(i)))
                return;

        return;
    }

    //Add directory entry to tree widget item (parent)
    QTreeWidgetItem *item = new QTreeWidgetItem({QString::fromUtf8(file->filename), file->is_dir ? "" : naturalSize(file->size)});
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsEditable | Qt::ItemIsEnabled);
    item->setData(0, Qt::UserRole, QVariant(file->is_dir));
    item->setData(1, Qt::UserRole, QVariant(false));
    item->setData(2, Qt::UserRole, QVariant(QString::fromUtf8(file->filename)));
    emit main_window->wantToAddTreeItem(item, w);
}

void MainWindow::usblink_dirlist_callback(struct usblink_file *file, void *data)
{
    QTreeWidget *w = static_cast<QTreeWidget*>(data);

    //End of enumeration or error
    if(!file)
    {
        //Find a dir to fill with entries
        for(int i = 0; i < w->topLevelItemCount(); ++i)
            if(usblink_dirlist_nested(w->topLevelItem(i)))
                return;

        return;
    }

    //Add directory entry to tree widget
    QTreeWidgetItem *item = new QTreeWidgetItem({QString::fromUtf8(file->filename), file->is_dir ? "" : naturalSize(file->size)});
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsEditable | Qt::ItemIsEnabled);
    item->setData(0, Qt::UserRole, QVariant(file->is_dir));
    item->setData(1, Qt::UserRole, QVariant(false));
    item->setData(2, Qt::UserRole, QVariant(QString::fromUtf8(file->filename)));
    emit main_window->wantToAddTreeItem(item, nullptr);
}

void MainWindow::usblink_delete_callback(int progress, void *data)
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

void MainWindow::usblink_download_callback(int progress, void *data)
{
    usblink_progress_callback(progress, data);

    if(progress < 0)
    {
        MainWindow *mw = static_cast<MainWindow*>(data);
        QMessageBox::warning(mw, tr("Download failed"), tr("Could not download file."));
    }
}

void MainWindow::usblink_progress_callback(int progress, void *data)
{
    if(progress < 0 || progress > 100)
        progress = 0; //No error handling here

    MainWindow *mw = static_cast<MainWindow*>(data);
    emit mw->usblink_progress_changed(progress);
}

void MainWindow::suspendToPath(QString path)
{
    emu_thread->suspend(path.toStdString());
}

void MainWindow::resumeFromPath(QString path)
{
    if(!emu_thread->resume(path.toStdString()))
        QMessageBox::warning(this, tr("Could not resume"), tr("Try to restart this app."));
}

void MainWindow::reload_filebrowser()
{
    if(!usblink_connected)
        usblink_connect();

    ui->treeWidget->clear();
    usblink_queue_dirlist("/", usblink_dirlist_callback, ui->treeWidget);
}

void MainWindow::changeProgress(int value)
{
    ui->progressBar->setValue(value);
}

static void usblink_move_progress(int progress, void *user_data)
{
    auto *item = reinterpret_cast<QTreeWidgetItem*>(user_data);

    if(progress == 100) // Success
        item->setData(2, Qt::UserRole, item->data(0, Qt::DisplayRole)); // Set internal name to new name
    else if(progress < 0) // Failure
        item->setData(0, Qt::DisplayRole, item->data(2, Qt::UserRole)); // Reset display name to old name
}

void MainWindow::usblink_dirlist_dataChanged(QTreeWidgetItem *item, int column)
{
    // Only the name can be changed
    if(column != 0)
        return;

    std::string filepath = usblink_path_item(item->parent()).toStdString();

    std::string old_name = item->data(2, Qt::UserRole).toString().toStdString(),
             new_name = item->data(0, Qt::DisplayRole).toString().toStdString();

    usblink_queue_move(filepath + "/" + old_name, filepath + "/" + new_name, usblink_move_progress, item);
}

void MainWindow::usblinkContextMenu(QPoint pos)
{
    if(!ui->treeWidget->currentItem())
        return;

    QMenu *menu = new QMenu(this);
    QAction *action_delete = new QAction(tr("Delete"), menu);

    if(ui->treeWidget->currentItem()->data(0, Qt::UserRole).toBool() == false)
    {
        // Is not a directory
        QAction *action_download = new QAction(tr("Download"), menu);
        connect(action_download, SIGNAL(triggered()), this, SLOT(usblinkDownloadEntry()));
        menu->addAction(action_download);
    }
    else if(ui->treeWidget->currentItem()->childCount() > 0)
    {
        // Non-empty directory
        action_delete->setDisabled(true);
    }

    connect(action_delete, SIGNAL(triggered()), this, SLOT(usblinkDeleteEntry()));
    menu->addAction(action_delete);

    menu->popup(ui->treeWidget->viewport()->mapToGlobal(pos));
}

void MainWindow::usblinkDownloadEntry()
{
    if(!ui->treeWidget->currentItem()
            || ui->treeWidget->currentItem()->data(0, Qt::UserRole).toBool()) // Is a directory
        return;

    QString dest = QFileDialog::getSaveFileName(this, tr("Chose save location"), QString(), tr("TNS file (*.tns)"));
    if(!dest.isEmpty())
        usblink_queue_download(usblink_path_item(ui->treeWidget->currentItem()).toStdString(), dest.toStdString(), usblink_download_callback, this);
}

void MainWindow::usblinkDeleteEntry()
{
    if(!ui->treeWidget->currentItem())
        return;

    usblink_queue_delete(usblink_path_item(ui->treeWidget->currentItem()).toStdString(), ui->treeWidget->currentItem()->data(0, Qt::UserRole).toBool(), usblink_delete_callback, ui->treeWidget->currentItem());
}

void MainWindow::addTreeItem(QTreeWidgetItem *item, QTreeWidgetItem *parent)
{
    if(parent)
        parent->addChild(item);
    else
        ui->treeWidget->addTopLevelItem(item);
}

void MainWindow::selectBoot1(QString path)
{
    QFileInfo f(path);
    emu.boot1 = path.toStdString();
    ui->filenameBoot1->setText(f.fileName());

    settings->setValue("boot1", path);
}

void MainWindow::selectBoot1()
{
    QFileInfo f(QString::fromStdString(emu.flash));
    QString path = QFileDialog::getOpenFileName(this, tr("Select boot1 file"), f.dir().absolutePath());
    if(!path.isNull())
        selectBoot1(path);
}

void MainWindow::selectFlash(QString path)
{
    QFileInfo f(path);
    emu.flash = path.toStdString();
    ui->filenameFlash->setText(f.fileName());

    settings->setValue("flash", path);
}

void MainWindow::updateUIActionState(bool emulation_running)
{
    ui->actionReset->setEnabled(emulation_running);
    ui->actionPause->setEnabled(emulation_running);
    ui->actionRestart->setText(emulation_running ? tr("Re&start") : tr("&Start"));

    ui->actionScreenshot->setEnabled(emulation_running);
    ui->actionRecord_GIF->setEnabled(emulation_running);
    ui->actionConnect->setEnabled(emulation_running);
    ui->actionDebugger->setEnabled(emulation_running);
    ui->actionXModem->setEnabled(emulation_running);

    ui->actionSuspend->setEnabled(emulation_running);
    ui->actionSuspend_to_file->setEnabled(emulation_running);
    ui->actionSave->setEnabled(emulation_running);
}

void MainWindow::raiseDebugger()
{
    if(dock_debugger)
    {
        dock_debugger->setVisible(true);
        dock_debugger->raise();
    }
    ui->tabWidget->setCurrentWidget(ui->tabDebugger);
}

void MainWindow::selectFlash()
{
    QFileInfo f(QString::fromStdString(emu.flash));
    QString path = QFileDialog::getOpenFileName(this, tr("Select flash file"), f.dir().absolutePath());
    if(!path.isNull())
        selectFlash(path);
}

void MainWindow::setDebuggerOnStartup(bool b)
{
    debug_on_start = b;
    settings->setValue("debugOnStart", b);
    if(ui->checkDebugger->isChecked() != b)
        ui->checkDebugger->setChecked(b);
}

void MainWindow::setDebuggerOnWarning(bool b)
{
    debug_on_warn = b;
    settings->setValue("debugOnWarn", b);
    if(ui->checkWarning->isChecked() != b)
        ui->checkWarning->setChecked(b);
}

void MainWindow::setUIMode(bool docks_enabled)
{
    // Already in this mode?
    if(docks_enabled == ui->tabWidget->isHidden())
        return;

    settings->setValue("docksEnabled", docks_enabled);

    // Enabling tabs needs a restart
    if(!docks_enabled)
    {
        QMessageBox::warning(this, trUtf8("Restart needed"), trUtf8("You need to restart firebird to enable the tab interface."));
        return;
    }

    // Create "Docks" menu to make closing and opening docks more intuitive
    QMenu *docks_menu = new QMenu(tr("Docks"));
    ui->menubar->insertMenu(ui->menuAbout->menuAction(), docks_menu);

    //Convert the tabs into QDockWidgets
    QDockWidget *last_dock = nullptr;
    while(ui->tabWidget->count())
    {
        QDockWidget *dw = new QDockWidget(ui->tabWidget->tabText(0));
        dw->setWindowIcon(ui->tabWidget->tabIcon(0));
        dw->setObjectName(dw->windowTitle());

        // Fill "Docks" menu
        QAction *action = dw->toggleViewAction();
        action->setIcon(dw->windowIcon());
        docks_menu->addAction(action);

        QWidget *tab = ui->tabWidget->widget(0);
        if(tab == ui->tabDebugger)
            dock_debugger = dw;

        dw->setWidget(tab);

        addDockWidget(Qt::RightDockWidgetArea, dw);
        if(last_dock != nullptr)
            tabifyDockWidget(last_dock, dw);

        last_dock = dw;
    }

    ui->tabWidget->setHidden(true);
    ui->uiDocks->setChecked(docks_enabled);
}

void MainWindow::setAutostart(bool b)
{
    settings->setValue("emuAutostart", b);
    if(ui->checkAutostart->isChecked() != b)
        ui->checkAutostart->setChecked(b);
}

void MainWindow::setBootOrder(bool diags_first)
{
    boot_order = diags_first ? ORDER_DIAGS : ORDER_BOOT2;
}

void MainWindow::setUSBPath(QString path)
{
    settings->setValue("usbdir", path);
    ln_target_folder = path.toStdString(); // For the "ln" debug cmd
    if(ui->pathTransfer->text() != path)
        ui->pathTransfer->setText(path);
}

void MainWindow::setGDBPort(int port)
{
    settings->setValue("gdbPort", port);
    emu_thread->port_gdb = port;
    //valueChanged signal will only be emitted if the value actually changed
    ui->spinGDB->setValue(port);
}

void MainWindow::setRDBGPort(int port)
{
    settings->setValue("rdbgPort", port);
    emu_thread->port_rdbg = port;
    //valueChanged signal will only be emitted if the value actually changed
    ui->spinRDBG->setValue(port);
}

void MainWindow::setSuspendOnClose(bool b)
{
    settings->setValue("suspendOnClose", b);
}

void MainWindow::setResumeOnOpen(bool b)
{
    settings->setValue("resumeOnOpen", b);
}

void MainWindow::changeSnapshotPath()
{
    QString path = QFileDialog::getSaveFileName(this, tr("Select snapshot location"));
    if(path.isNull())
        return;

    settings->setValue("snapshotPath", path);
    ui->labelSnapshotPath->setText(path);
}

void MainWindow::showSpeed(double value)
{
    ui->buttonSpeed->setText(tr("Speed: %1 %").arg(value * 100, 1, 'f', 0));
}

void MainWindow::flashCreated(QString path)
{
    if(QMessageBox::question(this, tr("Apply new flash?"), tr("Do you want to work with the newly created flash image now?")) == QMessageBox::Yes)
        this->selectFlash(path);
}

void MainWindow::screenshot()
{
    QImage image = renderFramebuffer();

    QString filename = QFileDialog::getSaveFileName(this, tr("Save Screenshot"), QString(), "PNG images (*.png)");
    if(filename.isNull())
        return;

    if(!image.save(filename, "PNG"))
        QMessageBox::critical(this, tr("Screenshot failed"), tr("Failed to save screenshot!"));
}

void MainWindow::recordGIF()
{
    static QString path;

    if(path.isEmpty())
    {
        // TODO: Use QTemporaryFile?
        path = QDir::tempPath() + QDir::separator() + "firebird_tmp.gif";

        gif_start_recording(path.toStdString().c_str(), 3);
    }
    else
    {
        if(gif_stop_recording())
        {
            QString filename = QFileDialog::getSaveFileName(this, tr("Save Screenshot"), QString(), "GIF images (*.gif)");
            if(filename.isNull())
                QFile(path).remove();
            else
                QFile(path).rename(filename);
        }
        else
            QMessageBox::warning(this, tr("Failed recording GIF"), tr("A failure occured during recording"));

        path = "";
    }

    ui->actionRecord_GIF->setChecked(!path.isEmpty());
}

void MainWindow::connectUSB()
{
    if(usblink_connected)
        usblink_queue_reset();
    else
        usblink_connect();

    usblinkChanged(false);
}

void MainWindow::usblinkChanged(bool state)
{
    ui->actionConnect->setText(state ? tr("Disconnect USB") : tr("Connect USB"));
    ui->actionConnect->setChecked(state);
    ui->buttonUSB->setText(state ? tr("Disconnect USB") : tr("Connect USB"));
    ui->buttonUSB->setChecked(state);
}

void MainWindow::resume()
{
    QString default_snapshot = settings->value("snapshotPath").toString();
    if(!default_snapshot.isEmpty())
        resumeFromPath(default_snapshot);
    else
        QMessageBox::warning(this, tr("Can't resume"), tr("No snapshot path (Settings->Snapshot) given"));
}

void MainWindow::suspend()
{
    QString default_snapshot = settings->value("snapshotPath").toString();
    if(!default_snapshot.isEmpty())
        suspendToPath(default_snapshot);
    else
        QMessageBox::warning(this, tr("Can't suspend"), tr("No snapshot path (Settings->Snapshot) given"));
}

void MainWindow::resumeFromFile()
{
    QString snapshot = QFileDialog::getOpenFileName(this, tr("Select snapshot to resume from"));
    if(!snapshot.isEmpty())
        resumeFromPath(snapshot);
}

void MainWindow::suspendToFile()
{
    QString snapshot = QFileDialog::getSaveFileName(this, tr("Select snapshot to suspend to"));
    if(!snapshot.isEmpty())
        suspendToPath(snapshot);
}

void MainWindow::saveFlash()
{
    flash_save_changes();
}

void MainWindow::createFlash()
{
    flash_dialog.show();
    flash_dialog.exec();
}


void MainWindow::showAbout()
{
    #define STRINGIFYMAGIC(x) #x
    #define STRINGIFY(x) STRINGIFYMAGIC(x)
    QMessageBox about_box(this);
    about_box.addButton(QMessageBox::Ok);
    about_box.setIconPixmap(about_box.windowIcon().pixmap(about_box.windowIcon().actualSize(QSize(64, 64))));
    about_box.setWindowTitle(tr("About Firebird"));
    about_box.setText(tr("<h3>Firebird %1</h3>"
                         "<a href='https://github.com/nspire-emus/firebird'>On GitHub</a><br><br>"
                         "Authors:<br>"
                         "Fabian Vogt (<a href='https://github.com/Vogtinator'>Vogtinator</a>)<br>"
                         "Adrien Bertrand (<a href='https://github.com/adriweb'>Adriweb</a>)<br>"
                         "Antonio Vasquez (<a href='https://github.com/antoniovazquezblanco'>antoniovazquezblanco</a>)<br>"
                         "Lionel Debroux (<a href='https://github.com/debrouxl'>debrouxl</a>)<br>"
                         "Based on nspire_emu v0.70 by Goplat<br><br>"
                         "This work is licensed under the GPLv3.<br>"
                         "To view a copy of this license, visit <a href='https://www.gnu.org/licenses/gpl-3.0.html'>https://www.gnu.org/licenses/gpl-3.0.html</a>").arg(STRINGIFY(FB_VERSION)));
    about_box.setTextFormat(Qt::RichText);
    about_box.show();
    about_box.exec();
    #undef STRINGIFY
    #undef STRINGIFYMAGIC
}

void MainWindow::started(bool success)
{
    updateUIActionState(success);

    if(success)
        showStatusMsg(tr("Emulation started."));
    else
        QMessageBox::warning(this, tr("Could not start the emulation"), tr("Starting the emulation failed.\nAre the paths to boot1 and flash correct?"));
}

void MainWindow::resumed(bool success)
{
    updateUIActionState(success);

    if(success)
        showStatusMsg(tr("Emulation resumed from snapshot."));
    else
        QMessageBox::warning(this, tr("Could not resume"), tr("Resuming failed.\nTry to fix the issue and try again."));
}

void MainWindow::suspended(bool success)
{
    if(success)
        showStatusMsg(tr("Snapshot saved."));
    else
        QMessageBox::warning(this, tr("Could not suspend"), tr("Suspending failed.\nTry to fix the issue and try again."));

    if(close_after_suspend)
    {
        if(!success)
            close_after_suspend = false; // May try again
        else
            this->close();
    }
}

void MainWindow::stopped()
{
    updateUIActionState(false);
    showStatusMsg(tr("Emulation stopped."));
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    if(!close_after_suspend &&
            settings->value("suspendOnClose").toBool() && emu_thread->isRunning() && exiting == false)
    {
        close_after_suspend = true;
        qDebug("Suspending...");
        suspend();
        e->ignore();
        return;
    }

    if(emu.isRunning() && !emu.stop())
        qDebug("Terminating emulator thread failed.");

    QMainWindow::closeEvent(e);
}

void MainWindow::showStatusMsg(QString str)
{
    status_label.setText(str);
}

void MainWindow::restart()
{
    if(emu.boot1 == "")
    {
        QMessageBox::critical(this, trUtf8("No boot1 set"), trUtf8("Before you can start the emulation, you have to select a proper boot1 file."));
        selectBoot1();
        return;
    }

    if(emu.flash == "")
    {
        QMessageBox::critical(this, trUtf8("No flash image loaded"), trUtf8("Before you can start the emulation, you have to load a proper flash file.\n"
                                                                            "You can create one via Flash->Create Flash in the menu."));
        return;
    }

    if(emu.stop())
        emu.start();
    else
        QMessageBox::warning(this, trUtf8("Restart needed"), trUtf8("Failed to restart emulator. Close and reopen this app.\n"));
}

void MainWindow::xmodemSend()
{
    QString filename = QFileDialog::getOpenFileName(this, tr("Select file to send"));
    if(filename.isNull())
        return;

    std::string path = filename.toStdString();
    xmodem_send(path.c_str());
}
