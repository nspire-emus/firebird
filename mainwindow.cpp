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
    connect(&emu, &EmuThread::serialChar, this, &MainWindow::serialChar, Qt::QueuedConnection);
    connect(&emu, &EmuThread::debugStr, this, &MainWindow::debugStr); //Not queued connection as it may cause a hang
    connect(&emu, &EmuThread::speedChanged, this, &MainWindow::showSpeed, Qt::QueuedConnection);
    connect(&emu, &EmuThread::statusMsg, this, &MainWindow::showStatusMsg, Qt::QueuedConnection);
    connect(&emu, &EmuThread::turboModeChanged, ui->buttonSpeed, &QPushButton::setChecked, Qt::QueuedConnection);
    connect(&emu, &EmuThread::usblinkChanged, this, &MainWindow::usblinkChanged, Qt::QueuedConnection);
    connect(&emu, &EmuThread::debugInputRequested, this, &MainWindow::debugInputRequested, Qt::QueuedConnection);
    connect(&emu, &EmuThread::started, this, &MainWindow::started, Qt::QueuedConnection);
    connect(&emu, &EmuThread::paused, ui->actionPause, &QAction::setChecked, Qt::QueuedConnection);
    connect(&emu, &EmuThread::resumed, this, &MainWindow::resumed, Qt::QueuedConnection);
    connect(&emu, &EmuThread::suspended, this, &MainWindow::suspended, Qt::QueuedConnection);
    connect(&emu, &EmuThread::stopped, this, &MainWindow::stopped, Qt::QueuedConnection);

    //GUI -> Emu (no QueuedConnection possible, watch out!)
    connect(this, &MainWindow::debuggerCommand, &emu, &EmuThread::debuggerInput);

    //Menu "Emulator"
    connect(ui->buttonReset, &QPushButton::clicked, &emu, &EmuThread::reset);
    connect(ui->actionReset, &QAction::triggered, &emu, &EmuThread::reset);
    connect(ui->actionRestart, &QAction::triggered, this, &MainWindow::restart);
    connect(ui->actionDebugger, &QAction::triggered, &emu, &EmuThread::enterDebugger);
    connect(ui->buttonPause, &QPushButton::clicked, &emu, &EmuThread::setPaused);
    connect(ui->buttonPause, &QPushButton::clicked, ui->actionPause, &QAction::setChecked);
    connect(ui->actionPause, &QAction::toggled, &emu, &EmuThread::setPaused);
    connect(ui->actionPause, &QAction::toggled, ui->buttonPause, &QPushButton::setChecked);
    connect(ui->buttonSpeed, &QPushButton::clicked, &emu, &EmuThread::setTurboMode);
    QShortcut *shortcut = new QShortcut(QKeySequence(Qt::Key_F11), this);
    shortcut->setAutoRepeat(false);
    connect(shortcut, &QShortcut::activated, &emu, &EmuThread::toggleTurbo);

    //Menu "Tools"
    connect(ui->buttonScreenshot, &QPushButton::clicked, this, &MainWindow::screenshot);
    connect(ui->actionScreenshot, &QAction::triggered, this, &MainWindow::screenshot);
    connect(ui->actionRecord_GIF, &QAction::triggered, this, &MainWindow::recordGIF);
    connect(ui->actionConnect, &QAction::triggered, this, &MainWindow::connectUSB);
    connect(ui->buttonUSB, &QPushButton::clicked, this, &MainWindow::connectUSB);
    connect(ui->actionXModem, &QAction::triggered, this, &MainWindow::xmodemSend);
    ui->actionConnect->setShortcut(QKeySequence(Qt::Key_F10));
    ui->actionConnect->setAutoRepeat(false);

    //Menu "State"
    connect(ui->actionResume, &QAction::triggered, this, &MainWindow::resume);
    connect(ui->actionSuspend, &QAction::triggered, this, &MainWindow::suspend);
    connect(ui->actionResume_from_file, &QAction::triggered, this, &MainWindow::resumeFromFile);
    connect(ui->actionSuspend_to_file, &QAction::triggered, this, &MainWindow::suspendToFile);

    //Menu "Flash"
    connect(ui->actionSave, &QAction::triggered, this, &MainWindow::saveFlash);
    connect(ui->actionCreate_flash, &QAction::triggered, this, &MainWindow::createFlash);

    //Menu "About"
    connect(ui->actionAbout_Firebird, &QAction::triggered, this, &MainWindow::showAbout);
    connect(ui->actionAbout_Qt, &QAction::triggered, qApp, &QApplication::aboutQt);

    //Debugging
    connect(ui->lineEdit, &QLineEdit::returnPressed, this, &MainWindow::debugCommand);

    //File transfer
    connect(ui->refreshButton, &QPushButton::clicked, ui->usblinkTree, &USBLinkTreeWidget::reloadFilebrowser);
    connect(ui->usblinkTree, &USBLinkTreeWidget::downloadProgress, this, &MainWindow::usblinkDownload, Qt::QueuedConnection);
    connect(ui->usblinkTree, &USBLinkTreeWidget::uploadProgress, this, &MainWindow::changeProgress, Qt::QueuedConnection);
    connect(this, &MainWindow::usblink_progress_changed, this, &MainWindow::changeProgress, Qt::QueuedConnection);

    //Settings
    connect(ui->checkDebugger, &QCheckBox::toggled, this, &MainWindow::setDebuggerOnStartup);
    connect(ui->checkWarning, &QCheckBox::toggled, this, &MainWindow::setDebuggerOnWarning);
    connect(ui->uiDocks, &QRadioButton::toggled, this, &MainWindow::setUIMode);
    connect(ui->checkAutostart, &QCheckBox::toggled, this, &MainWindow::setAutostart);
    connect(ui->fileBoot1, &QPushButton::pressed, this, &MainWindow::selectBoot1);
    connect(ui->fileFlash, &QPushButton::pressed, this, &MainWindow::selectFlash);
    connect(ui->pathTransfer, &QLineEdit::textEdited, this, &MainWindow::setUSBPath);
    connect(ui->spinGDB, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &MainWindow::setGDBPort);
    connect(ui->spinRDBG, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &MainWindow::setRDBGPort);
    connect(ui->orderDiags, &QRadioButton::toggled, this, &MainWindow::setBootOrder);
    connect(ui->checkSuspend, &QCheckBox::toggled, this, &MainWindow::setSuspendOnClose);
    connect(ui->checkResume, &QCheckBox::toggled, this, &MainWindow::setResumeOnOpen);
    connect(ui->buttonChangeSnapshotPath, &QPushButton::clicked, this, &MainWindow::changeSnapshotPath);

    //FlashDialog
    connect(&flash_dialog, &FlashDialog::flashCreated, this, &MainWindow::flashCreated);

    //Set up monospace fonts
    QFont monospace = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    ui->debugConsole->setFont(monospace);
    ui->serialConsole->setFont(monospace);

    qRegisterMetaType<QVector<int>>();

#ifdef Q_OS_ANDROID
    //On android the settings file is deleted everytime you update or uninstall,
    //so choose a better, safer, location
    QString path = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    settings = new QSettings(path + QStringLiteral("/nspire_emu.ini"), QSettings::IniFormat);
#else
    settings = new QSettings();
#endif

    updateUIActionState(false);

    //Migrate old settings
    if(settings->value(QStringLiteral("usbdirNew"), QString()).toString().isEmpty())
        settings->setValue(QStringLiteral("usbdirNew"), QStringLiteral("/") + settings->value(QStringLiteral("usbdir"), QStringLiteral("ndless")).toString());

    //Load settings
    setUIMode(settings->value(QStringLiteral("docksEnabled"), true).toBool());
    restoreGeometry(settings->value(QStringLiteral("windowGeometry")).toByteArray());
    restoreState(settings->value(QStringLiteral("windowState")).toByteArray(), WindowStateVersion);
    setPathBoot1(settings->value(QStringLiteral("boot1"), QStringLiteral("")).toString());
    setPathFlash(settings->value(QStringLiteral("flash"), QStringLiteral("")).toString());
    setDebuggerOnStartup(settings->value(QStringLiteral("debugOnStart"), false).toBool());
    setDebuggerOnWarning(settings->value(QStringLiteral("debugOnWarn"), false).toBool());
    setUSBPath(settings->value(QStringLiteral("usbdirNew"), QStringLiteral("/ndless")).toString());
    setGDBPort(settings->value(QStringLiteral("gdbPort"), 3333).toUInt());
    setRDBGPort(settings->value(QStringLiteral("rdbgPort"), 3334).toUInt());
    ui->checkSuspend->setChecked(settings->value(QStringLiteral("suspendOnClose"), false).toBool());
    ui->checkResume->setChecked(settings->value(QStringLiteral("resumeOnOpen"), false).toBool());
    if(!settings->value(QStringLiteral("snapshotPath")).toString().isEmpty())
        ui->labelSnapshotPath->setText(settings->value(QStringLiteral("snapshotPath")).toString());

    setBootOrder(false);

    if(settings->value(QStringLiteral("resumeOnOpen")).toBool())
        resume();
    else
    {
        bool autostart = settings->value(QStringLiteral("emuAutostart"), false).toBool();
        setAutostart(autostart);
        if(!emu.boot1.isEmpty() && !emu.flash.isEmpty() && autostart)
            emu.start();
        else
            showStatusMsg(tr("Start the emulation via Emulation->Restart."));
    }

    ui->lcdView->setFocus();
}

MainWindow::~MainWindow()
{
    settings->setValue(QStringLiteral("windowState"), saveState(WindowStateVersion));
    settings->setValue(QStringLiteral("windowGeometry"), saveGeometry());

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
        usblink_queue_put_file(local.toString().toStdString(), settings->value(QStringLiteral("usbdirNew"), QStringLiteral("/ndless")).toString().toStdString(), usblink_progress_callback, this);
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent *e)
{
    if(e->mimeData()->hasUrls() == false)
        return e->ignore();

    for(QUrl &url : e->mimeData()->urls())
        if(!url.fileName().endsWith(QStringLiteral(".tns")))
            return e->ignore();

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
            if(previous == '\r' && c != '\n')
            {
                ui->serialConsole->moveCursor(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);
                ui->serialConsole->moveCursor(QTextCursor::End, QTextCursor::KeepAnchor);
                ui->serialConsole->textCursor().removeSelectedText();
                previous = 0;
            }
            ui->serialConsole->insertPlainText(QChar::fromLatin1(c));
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

void MainWindow::usblinkDownload(int progress)
{
    usblinkProgress(progress);

    if(progress < 0)
        QMessageBox::warning(this, tr("Download failed"), tr("Could not download file."));
}

void MainWindow::usblinkProgress(int progress)
{
    if(progress < 0 || progress > 100)
        progress = 0; //No error handling here

    emit usblink_progress_changed(progress);
}

void MainWindow::usblink_progress_callback(int progress, void *)
{
    if(progress < 0 || progress > 100)
        progress = 0; //No error handling here

    emit main_window->usblink_progress_changed(progress);
}

void MainWindow::suspendToPath(QString path)
{
    emu_thread->suspend(path);
}

void MainWindow::resumeFromPath(QString path)
{
    if(!emu_thread->resume(path))
        QMessageBox::warning(this, tr("Could not resume"), tr("Try to restart this app."));
}

void MainWindow::changeProgress(int value)
{
    ui->progressBar->setValue(value);
}

void MainWindow::setPathBoot1(QString path)
{
    QFileInfo f(path);
    emu.boot1 = path;
    ui->filenameBoot1->setText(f.fileName());

    settings->setValue(QStringLiteral("boot1"), path);
}

void MainWindow::selectBoot1()
{
    QFileInfo f(emu.flash);
    QString path = QFileDialog::getOpenFileName(this, tr("Select boot1 file"), f.dir().absolutePath());
    if(!path.isEmpty())
        setPathBoot1(path);
}

void MainWindow::setPathFlash(QString path)
{
    QFileInfo f(path);
    emu.flash = path;
    ui->filenameFlash->setText(f.fileName());

    settings->setValue(QStringLiteral("flash"), path);
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
    QFileInfo f(emu.flash);
    QString path = QFileDialog::getOpenFileName(this, tr("Select flash file"), f.dir().absolutePath());
    if(!path.isEmpty())
        setPathFlash(path);
}

void MainWindow::setDebuggerOnStartup(bool b)
{
    debug_on_start = b;
    settings->setValue(QStringLiteral("debugOnStart"), b);
    if(ui->checkDebugger->isChecked() != b)
        ui->checkDebugger->setChecked(b);
}

void MainWindow::setDebuggerOnWarning(bool b)
{
    debug_on_warn = b;
    settings->setValue(QStringLiteral("debugOnWarn"), b);
    if(ui->checkWarning->isChecked() != b)
        ui->checkWarning->setChecked(b);
}

void MainWindow::setUIMode(bool docks_enabled)
{
    // Already in this mode?
    if(docks_enabled == ui->tabWidget->isHidden())
        return;

    settings->setValue(QStringLiteral("docksEnabled"), docks_enabled);

    // Enabling tabs needs a restart
    if(!docks_enabled)
    {
        QMessageBox::warning(this, trUtf8("Restart needed"), trUtf8("You need to restart firebird to enable the tab interface."));
        return;
    }

    // Create "Docks" menu to make closing and opening docks more intuitive
    QMenu *docks_menu = new QMenu(tr("Docks"), this);
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
    settings->setValue(QStringLiteral("emuAutostart"), b);
    if(ui->checkAutostart->isChecked() != b)
        ui->checkAutostart->setChecked(b);
}

void MainWindow::setBootOrder(bool diags_first)
{
    boot_order = diags_first ? ORDER_DIAGS : ORDER_BOOT2;
}

void MainWindow::setUSBPath(QString path)
{
    settings->setValue(QStringLiteral("/usbdir"), path);
    ln_target_folder = path.toStdString(); // For the "ln" debug cmd
    if(ui->pathTransfer->text() != path)
        ui->pathTransfer->setText(path);
}

void MainWindow::setGDBPort(int port)
{
    settings->setValue(QStringLiteral("gdbPort"), port);
    emu_thread->port_gdb = port;
    //valueChanged signal will only be emitted if the value actually changed
    ui->spinGDB->setValue(port);
}

void MainWindow::setRDBGPort(int port)
{
    settings->setValue(QStringLiteral("rdbgPort"), port);
    emu_thread->port_rdbg = port;
    //valueChanged signal will only be emitted if the value actually changed
    ui->spinRDBG->setValue(port);
}

void MainWindow::setSuspendOnClose(bool b)
{
    settings->setValue(QStringLiteral("suspendOnClose"), b);
}

void MainWindow::setResumeOnOpen(bool b)
{
    settings->setValue(QStringLiteral("resumeOnOpen"), b);
}

void MainWindow::changeSnapshotPath()
{
    QString path = QFileDialog::getSaveFileName(this, tr("Select snapshot location"));
    if(path.isEmpty())
        return;

    settings->setValue(QStringLiteral("snapshotPath"), path);
    ui->labelSnapshotPath->setText(path);
}

void MainWindow::showSpeed(double value)
{
    ui->buttonSpeed->setText(tr("Speed: %1 %").arg(value * 100, 1, 'f', 0));
}

void MainWindow::flashCreated(QString path)
{
    if(QMessageBox::question(this, tr("Apply new flash?"), tr("Do you want to work with the newly created flash image now?")) == QMessageBox::Yes)
        this->setPathFlash(path);
}

void MainWindow::screenshot()
{
    QImage image = renderFramebuffer();

    QString filename = QFileDialog::getSaveFileName(this, tr("Save Screenshot"), QString(), tr("PNG images (*.png)"));
    if(filename.isEmpty())
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
        path = QDir::tempPath() + QDir::separator() + QStringLiteral("firebird_tmp.gif");

        gif_start_recording(path.toStdString().c_str(), 3);
    }
    else
    {
        if(gif_stop_recording())
        {
            QString filename = QFileDialog::getSaveFileName(this, tr("Save Recording"), QString(), tr("GIF images (*.gif)"));
            if(filename.isEmpty())
                QFile(path).remove();
            else
                QFile(path).rename(filename);
        }
        else
            QMessageBox::warning(this, tr("Failed recording GIF"), tr("A failure occured during recording"));

        path = QString();
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
    QString default_snapshot = settings->value(QStringLiteral("snapshotPath")).toString();
    if(!default_snapshot.isEmpty())
        resumeFromPath(default_snapshot);
    else
        QMessageBox::warning(this, tr("Can't resume"), tr("No snapshot path (Settings->Snapshot) given"));
}

void MainWindow::suspend()
{
    QString default_snapshot = settings->value(QStringLiteral("snapshotPath")).toString();
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
                         "To view a copy of this license, visit <a href='https://www.gnu.org/licenses/gpl-3.0.html'>https://www.gnu.org/licenses/gpl-3.0.html</a>")
                         .arg(QStringLiteral(STRINGIFY(FB_VERSION))));
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
            settings->value(QStringLiteral("suspendOnClose")).toBool() && emu_thread->isRunning() && exiting == false)
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
    if(emu.boot1.isEmpty())
    {
        QMessageBox::critical(this, trUtf8("No boot1 set"), trUtf8("Before you can start the emulation, you have to select a proper boot1 file."));
        selectBoot1();
        return;
    }

    if(emu.flash.isEmpty())
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
    if(filename.isEmpty())
        return;

    std::string path = filename.toStdString();
    xmodem_send(path.c_str());
}
