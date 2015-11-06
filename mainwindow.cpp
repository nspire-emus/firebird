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

#include "core/usblink_queue.h"
#include "core/flash.h"
#include "core/emu.h"
#include "core/misc.h"

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "qmlbridge.h"
#include "qtframebuffer.h"
#include "qtkeypadbridge.h"

MainWindow *main_window;
bool MainWindow::refresh_filebrowser = true;
// Change this if you change the UI
static const constexpr int WindowStateVersion = 0;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Register QtKeypadBridge for the virtual keyboard functionality
    ui->keypadWidget->installEventFilter(&qt_keypad_bridge);
    ui->lcdView->installEventFilter(&qt_keypad_bridge);

    //Emu -> GUI (QueuedConnection as they're different threads)
    connect(&emu, SIGNAL(serialChar(char)), this, SLOT(serialChar(char)), Qt::QueuedConnection);
    connect(&emu, SIGNAL(debugStr(QString)), this, SLOT(debugStr(QString))); //Not queued connection as it may cause a hang
    connect(&emu, SIGNAL(speedChanged(double)), this, SLOT(showSpeed(double)), Qt::QueuedConnection);
    connect(&emu, SIGNAL(statusMsg(QString)), ui->statusbar, SLOT(showMessage(QString)), Qt::QueuedConnection);
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

    //Debugging
    connect(ui->lineEdit, SIGNAL(returnPressed()), this, SLOT(debugCommand()));

    //File transfer
    connect(ui->refreshButton, SIGNAL(clicked(bool)), this, SLOT(reload_filebrowser()));
    connect(this, SIGNAL(usblink_progress_changed(int)), this, SLOT(changeProgress(int)), Qt::QueuedConnection);

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
            ui->statusbar->showMessage(tr("Start the emulation via Emulation->Restart."));
    }

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
        refresh_filebrowser = true;

        w->setData(1, Qt::UserRole, QVariant(true)); //Dir is now filled
        //Find a dir to fill with entries
        for(int i = 0; i < w->treeWidget()->topLevelItemCount(); ++i)
            if(usblink_dirlist_nested(w->treeWidget()->topLevelItem(i)))
                return;

        return;
    }

    //Add directory entry to tree widget item (parent)
    QTreeWidgetItem *item = new QTreeWidgetItem({QString::fromUtf8(file->filename), file->is_dir ? "" : naturalSize(file->size)});
    item->setData(0, Qt::UserRole, QVariant(file->is_dir));
    item->setData(1, Qt::UserRole, QVariant(false));
    w->addChild(item);
}

void MainWindow::usblink_dirlist_callback(struct usblink_file *file, void *data)
{
    QTreeWidget *w = static_cast<QTreeWidget*>(data);

    //End of enumeration or error
    if(!file)
    {
        refresh_filebrowser = true;

        //Find a dir to fill with entries
        for(int i = 0; i < w->topLevelItemCount(); ++i)
            if(usblink_dirlist_nested(w->topLevelItem(i)))
                return;

        return;
    }

    //Add directory entry to tree widget
    QTreeWidgetItem *item = new QTreeWidgetItem({QString::fromUtf8(file->filename), file->is_dir ? "" : naturalSize(file->size)});
    item->setData(0, Qt::UserRole, QVariant(file->is_dir));
    item->setData(1, Qt::UserRole, QVariant(false));
    w->addTopLevelItem(item);
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
    if(!refresh_filebrowser)
        return;

    if(!usblink_connected)
        usblink_connect();

    ui->treeWidget->clear();
    usblink_queue_dirlist("/", usblink_dirlist_callback, ui->treeWidget);
}

void MainWindow::changeProgress(int value)
{
    ui->progressBar->setValue(value);
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
    ui->actionConnect->setEnabled(emulation_running);
    ui->actionDebugger->setEnabled(emulation_running);

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

    //Convert the tabs into QDockWidgets
    QDockWidget *last_dock = nullptr;
    while(ui->tabWidget->count())
    {
        QWidget *tab = ui->tabWidget->widget(0);
        QDockWidget *dw = new QDockWidget(ui->tabWidget->tabText(0), this);
        docks.push_back(dw);
        #ifdef Q_OS_MAC
            connect(dw, SIGNAL(visibilityChanged(bool)), this, SLOT(dockVisibilityChanged(bool)));
        #endif
        if(tab == ui->tabDebugger)
            dock_debugger = dw;

        dw->setObjectName(ui->tabWidget->tabText(0));
        tab->setParent(dw->widget());
        addDockWidget(Qt::RightDockWidgetArea, dw);
        dw->setWidget(tab);
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

void MainWindow::showSpeed(double percent)
{
    ui->buttonSpeed->setText(tr("Speed: %1 %").arg(percent, 1, 'f', 0));
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

void MainWindow::started(bool success)
{
    updateUIActionState(success);

    if(success)
        ui->statusbar->showMessage(tr("Emulation started."));
    else
        QMessageBox::warning(this, tr("Could not start the emulation"), tr("Starting the emulation failed.\nAre the paths to boot1 and flash correct?"));
}

void MainWindow::resumed(bool success)
{
    updateUIActionState(success);

    if(success)
        ui->statusbar->showMessage(tr("Emulation resumed from snapshot."));
    else
        QMessageBox::warning(this, tr("Could not resume"), tr("Resuming failed.\nTry to fix the issue and try again."));
}

void MainWindow::suspended(bool success)
{
    if(success)
        ui->statusbar->showMessage(tr("Snapshot saved."));
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
    ui->statusbar->showMessage(tr("Emulation stopped."));
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

    qDebug("Terminating emulator thread...");

    if(emu.stop())
        qDebug("Successful!");
    else
        qDebug("Failed.");

    QMainWindow::closeEvent(e);
}

// Workaround for bug on Mac: If all docks are hidden,
// it's not possible to open the dock menu and show them again
void MainWindow::dockVisibilityChanged(bool v)
{
    // A dock got visible: make all of them closable
    if(v)
    {
        for(auto dock : docks)
            dock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
        return;
    }

    // All but one dock hidden?
    QDockWidget *remaining = nullptr;
    for(auto dock : docks)
        if(dock->isVisible())
        {
            if(remaining)
                return; // Enough docks visible
            else
                remaining = dock;
        }

    if(!remaining)
        return; // No dock visible?

    // Disable closability on the remaining one
    remaining->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
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
