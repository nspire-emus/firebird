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
#include <QQmlComponent>

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
    lcd.installEventFilter(&qt_keypad_bridge);

    // Create config dialog
    ui->keypadWidget->engine()->addImportPath(QStringLiteral("qrc:/qml/qml"));
    config_dialog = (new QQmlComponent(ui->keypadWidget->engine(), QUrl(QStringLiteral("qrc:/qml/qml/FBConfigDialog.qml")), this))->create();

    //Emu -> GUI (QueuedConnection as they're different threads)
    connect(&emu, SIGNAL(serialChar(char)), this, SLOT(serialChar(char)), Qt::QueuedConnection);
    connect(&emu, SIGNAL(debugStr(QString)), this, SLOT(debugStr(QString))); //Not queued connection as it may cause a hang
    connect(&emu, SIGNAL(speedChanged(double)), this, SLOT(showSpeed(double)), Qt::QueuedConnection);
    connect(&emu, SIGNAL(isBusy(bool)), this, SLOT(isBusy(bool)), Qt::QueuedConnection);
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
    connect(ui->actionConfiguration, SIGNAL(triggered()), this, SLOT(openConfiguration()));
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
    connect(ui->actionLCD_Window, SIGNAL(triggered(bool)), this, SLOT(setExtLCD(bool)));
    connect(&lcd, SIGNAL(closed()), ui->actionLCD_Window, SLOT(toggle()));
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
    connect(ui->refreshButton, SIGNAL(clicked(bool)), ui->usblinkTree, SLOT(reloadFilebrowser()));
    connect(ui->usblinkTree, SIGNAL(downloadProgress(int)), this, SLOT(usblinkDownload(int)), Qt::QueuedConnection);
    connect(ui->usblinkTree, SIGNAL(uploadProgress(int)), this, SLOT(changeProgress(int)), Qt::QueuedConnection);
    connect(this, SIGNAL(usblink_progress_changed(int)), this, SLOT(changeProgress(int)), Qt::QueuedConnection);

    KitModel *model = the_qml_bridge->getKitModel();
    connect(model, SIGNAL(anythingChanged()), this, SLOT(kitAnythingChanged()));
    connect(model, SIGNAL(dataChanged(QModelIndex,QModelIndex,QVector<int>)), this, SLOT(kitDataChanged(QModelIndex,QModelIndex,QVector<int>)));

    //Set up monospace fonts
    QFont monospace = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    ui->debugConsole->setFont(monospace);
    ui->serialConsole->setFont(monospace);

    //Without this line, Qt prints warning messages...
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

    //Load settings
    setUIMode(true);
    setExtLCD(settings->value(QStringLiteral("extLCDVisible")).toBool());
    lcd.restoreGeometry(settings->value(QStringLiteral("extLCDGeometry")).toByteArray());
    restoreGeometry(settings->value(QStringLiteral("windowGeometry")).toByteArray());
    restoreState(settings->value(QStringLiteral("windowState")).toByteArray(), WindowStateVersion);

    snapshot_path = settings->value(QStringLiteral("snapshotPath")).toString();

    refillKitMenus();

    ui->lcdView->setFocus();

    if(!the_qml_bridge->getAutostart())
        return;

    // Autostart handling
    int kitIndex = the_qml_bridge->kitIndexForID(the_qml_bridge->getDefaultKit());
    if(kitIndex >= 0)
    {
        bool resumed = false;
        setCurrentKit(the_qml_bridge->getKitModel()->getKits()[kitIndex]);
        if(!snapshot_path.isEmpty())
            resumed = resume();

        if(!resumed)
        {
            // Boot up normally
            if(!emu.boot1.isEmpty() && !emu.flash.isEmpty())
                emu.start();
            else
                showStatusMsg(tr("Start the emulation via Emulation->Restart."));
        }
    }
}

MainWindow::~MainWindow()
{
    // Save external LCD geometry
    settings->setValue(QStringLiteral("extLCDGeometry"), lcd.saveGeometry());
    settings->setValue(QStringLiteral("extLCDVisible"), lcd.isVisible());

    // Save MainWindow state and geometry
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

    for(auto &&url : mime_data->urls())
    {
        QUrl local = url.toLocalFile();
        usblink_queue_put_file(local.toString().toStdString(), the_qml_bridge->getUSBDir().toStdString(), usblink_progress_callback, this);
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent *e)
{
    if(e->mimeData()->hasUrls() == false)
        return e->ignore();

    for(QUrl &url : e->mimeData()->urls())
    {
        static const QStringList valid_suffixes = { QStringLiteral("tns"), QStringLiteral("tno"),
                                              QStringLiteral("tnc"), QStringLiteral("tco"),
                                              QStringLiteral("tcc") };

        QFileInfo file(url.fileName());
        if(!valid_suffixes.contains(file.suffix().toLower()))
            return e->ignore();
    }

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
    // TODO: Don't do a full refresh
    // Also refresh on error, in case of multiple transfers
    if((progress == 100 || progress < 0) && usblink_queue_size() == 1)
        main_window->ui->usblinkTree->wantToReload(); // Reload the file explorer after uploads finished

    if(progress < 0 || progress > 100)
        progress = 0; //No error handling here

    emit main_window->usblink_progress_changed(progress);
}

void MainWindow::suspendToPath(QString path)
{
    emu_thread->suspend(path);
}

bool MainWindow::resumeFromPath(QString path)
{
    if(!emu_thread->resume(path))
    {
        QMessageBox::warning(this, tr("Could not resume"), tr("Try to restart this app."));
        return false;
    }

    return true;
}

void MainWindow::changeProgress(int value)
{
    ui->progressBar->setValue(value);
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

    ui->buttonSpeed->setEnabled(emulation_running);
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
}

void MainWindow::showSpeed(double value)
{
    ui->buttonSpeed->setText(tr("Speed: %1 %").arg(value * 100, 1, 'f', 0));
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
            {
                QFile(filename).remove();
                QFile(path).rename(filename);
            }
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

void MainWindow::setExtLCD(bool state)
{
    if(state)
        lcd.show();
    else
        lcd.hide();

    ui->actionLCD_Window->setChecked(state);
}

bool MainWindow::resume()
{
    applyQMLBridgeSettings();

    if(!snapshot_path.isEmpty())
        return resumeFromPath(snapshot_path);
    else
    {
        QMessageBox::warning(this, tr("Can't resume"), tr("The current kit does not have a snapshot file configured"));
        return false;
    }
}

void MainWindow::suspend()
{
    QString default_snapshot = settings->value(QStringLiteral("snapshotPath")).toString();
    if(!default_snapshot.isEmpty())
        suspendToPath(default_snapshot);
    else
        QMessageBox::warning(this, tr("Can't suspend"), tr("The current kit does not have a snapshot file configured"));
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
                         "Denis Avashurov (<a href='https://github.com/denisps'>denisps</a>)<br>"
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

void MainWindow::isBusy(bool busy)
{
    if(busy)
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    else
        QApplication::restoreOverrideCursor();
}

void MainWindow::started(bool success)
{
    updateUIActionState(success);

    if(success)
        showStatusMsg(tr("Emulation started"));
    else
        QMessageBox::warning(this, tr("Could not start the emulation"), tr("Starting the emulation failed.\nAre the paths to boot1 and flash correct?"));
}

void MainWindow::resumed(bool success)
{
    updateUIActionState(success);

    if(success)
        showStatusMsg(tr("Emulation resumed from snapshot"));
    else
        QMessageBox::warning(this, tr("Could not resume"), tr("Resuming failed.\nTry to fix the issue and try again."));
}

void MainWindow::suspended(bool success)
{
    if(success)
        showStatusMsg(tr("Snapshot saved"));
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
    showStatusMsg(tr("Emulation stopped"));
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
    qApp->exit(0);
}

void MainWindow::showStatusMsg(QString str)
{
    status_label.setText(str);
}

void MainWindow::kitDataChanged(QModelIndex, QModelIndex, QVector<int> roles)
{
    if(roles.contains(KitModel::NameRole))
        refillKitMenus();
}

void MainWindow::kitAnythingChanged()
{
    if(the_qml_bridge->getKitModel()->rowCount() != ui->menuRestart_with_Kit->actions().size())
        refillKitMenus();
}

void MainWindow::refillKitMenus()
{
    ui->menuRestart_with_Kit->clear();
    ui->menuBoot_Diags_with_Kit->clear();

    auto &&kit_model = the_qml_bridge->getKitModel();
    for(auto &&kit : kit_model->getKits())
    {
        // We can't use a reference as it would become dangling,
        // but can't use a value either, as it would need to be refreshed.
        // So link this using Kit IDs.
        unsigned int kit_id = kit.id;
        ui->menuRestart_with_Kit->addAction(kit.name, [=] {
            setCurrentKit(kit_model->getKits().at(kit_model->indexForID(kit_id)));
            boot_order = ORDER_BOOT2;
            restart();
        });
        ui->menuBoot_Diags_with_Kit->addAction(kit.name, [=] {
            setCurrentKit(kit_model->getKits().at(kit_model->indexForID(kit.id)));
            boot_order = ORDER_DIAGS;
            restart();
        });
    }
}

void MainWindow::applyQMLBridgeSettings()
{
    emu.port_gdb = the_qml_bridge->getGDBEnabled() ? the_qml_bridge->getGDBPort() : 0;
    emu.port_rdbg = the_qml_bridge->getRDBEnabled() ? the_qml_bridge->getRDBPort() : 0;
}

void MainWindow::setCurrentKit(const Kit &kit)
{
    emu.boot1 = kit.boot1;
    emu.flash = kit.flash;
    snapshot_path = kit.snapshot;
}

void MainWindow::restart()
{
    if(emu.boot1.isEmpty())
    {
        QMessageBox::critical(this, trUtf8("No boot1 set"), trUtf8("Before you can start the emulation, you have to select a proper boot1 file."));
        return;
    }

    if(emu.flash.isEmpty())
    {
        QMessageBox::critical(this, trUtf8("No flash image loaded"), trUtf8("Before you can start the emulation, you have to load a proper flash file.\n"
                                                                            "You can create one via Flash->Create Flash in the menu."));
        return;
    }

    applyQMLBridgeSettings();

    if(emu.stop())
        emu.start();
    else
        QMessageBox::warning(this, trUtf8("Restart needed"), trUtf8("Failed to restart emulator. Close and reopen this app.\n"));
}

void MainWindow::openConfiguration()
{
    config_dialog->setProperty("visible", QVariant(true));
}

void MainWindow::xmodemSend()
{
    QString filename = QFileDialog::getOpenFileName(this, tr("Select file to send"));
    if(filename.isEmpty())
        return;

    std::string path = filename.toStdString();
    xmodem_send(path.c_str());
}
