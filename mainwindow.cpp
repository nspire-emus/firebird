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

#include "dockwidget.h"
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

    ui->keypadWidget->setAttribute(Qt::WA_AcceptTouchEvents);

    qml_engine = ui->keypadWidget->engine();
    qml_engine->addImportPath(QStringLiteral("qrc:/qml/qml"));

    // Create config dialog component
    config_component = new QQmlComponent(qml_engine, QUrl(QStringLiteral("qrc:/qml/qml/FBConfigDialog.qml")), this);
    if(!config_component->isReady())
        qCritical() << "Could not create QML config dialog:" << config_component->errorString();

    // Create flash dialog UI component
    flash_dialog_component = new QQmlComponent(qml_engine, QUrl(QStringLiteral("qrc:/qml/qml/FlashDialog.qml")), this);
    if(!flash_dialog_component->isReady())
        qCritical() << "Could not create flash dialog component:" << flash_dialog_component->errorString();

    // Create mobile UI component
    mobileui_component = new QQmlComponent(qml_engine, QUrl(QStringLiteral("qrc:/qml/qml/MobileUI.qml")), this);
    if(!mobileui_component->isReady())
        qCritical() << "Could not create mobile UI component:" << mobileui_component->errorString();

    if(!the_qml_bridge)
        throw std::runtime_error("Can't continue without QMLBridge");

    //Emu -> GUI (QueuedConnection as they're different threads)
    connect(&emu_thread, SIGNAL(serialChar(char)), this, SLOT(serialChar(char)), Qt::QueuedConnection);
    connect(&emu_thread, SIGNAL(debugStr(QString)), this, SLOT(debugStr(QString)), Qt::QueuedConnection);
    connect(&emu_thread, SIGNAL(isBusy(bool)), this, SLOT(isBusy(bool)), Qt::QueuedConnection);
    connect(&emu_thread, SIGNAL(statusMsg(QString)), this, SLOT(showStatusMsg(QString)), Qt::QueuedConnection);
    connect(&emu_thread, SIGNAL(debugInputRequested(bool)), this, SLOT(debugInputRequested(bool)), Qt::QueuedConnection);

    //GUI -> Emu (no QueuedConnection possible, watch out!)
    connect(this, SIGNAL(debuggerCommand(QString)), &emu_thread, SLOT(debuggerInput(QString)));

    //Menu "Emulator"
    connect(ui->buttonReset, SIGNAL(clicked(bool)), &emu_thread, SLOT(reset()));
    connect(ui->actionReset, SIGNAL(triggered()), &emu_thread, SLOT(reset()));
    connect(ui->actionRestart, SIGNAL(triggered()), this, SLOT(restart()));
    connect(ui->actionDebugger, SIGNAL(triggered()), &emu_thread, SLOT(enterDebugger()));
    connect(ui->actionConfiguration, SIGNAL(triggered()), this, SLOT(openConfiguration()));
    connect(ui->buttonPause, SIGNAL(clicked(bool)), &emu_thread, SLOT(setPaused(bool)));
    connect(ui->buttonPause, SIGNAL(clicked(bool)), ui->actionPause, SLOT(setChecked(bool)));
    connect(ui->actionPause, SIGNAL(toggled(bool)), &emu_thread, SLOT(setPaused(bool)));
    connect(ui->actionPause, SIGNAL(toggled(bool)), ui->buttonPause, SLOT(setChecked(bool)));
    connect(ui->buttonSpeed, SIGNAL(clicked(bool)), &emu_thread, SLOT(setTurboMode(bool)));

    QShortcut *shortcut = new QShortcut(QKeySequence(Qt::Key_F11), this);
    shortcut->setAutoRepeat(false);
    connect(shortcut, SIGNAL(activated()), &emu_thread, SLOT(toggleTurbo()));

    //Menu "Tools"
    connect(ui->buttonScreenshot, SIGNAL(clicked()), this, SLOT(screenshot()));
    connect(ui->actionScreenshot, SIGNAL(triggered()), this, SLOT(screenshot()));
    connect(ui->actionRecord_GIF, SIGNAL(triggered()), this, SLOT(recordGIF()));
    connect(ui->actionConnect, SIGNAL(triggered()), this, SLOT(connectUSB()));
    connect(ui->buttonUSB, SIGNAL(clicked(bool)), this, SLOT(connectUSB()));
    connect(ui->actionLCD_Window, SIGNAL(triggered(bool)), this, SLOT(setExtLCD(bool)));
    connect(&lcd, SIGNAL(closed()), ui->actionLCD_Window, SLOT(toggle()));
    connect(ui->actionXModem, SIGNAL(triggered()), this, SLOT(xmodemSend()));
    connect(ui->actionSwitch_to_Mobile_UI, SIGNAL(triggered()), this, SLOT(switchToMobileUI()));
    connect(ui->actionLeavePTT, &QAction::triggered, the_qml_bridge, &QMLBridge::sendExitPTT);
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

    // Lang switch
    QStringList translations = QDir(QStringLiteral(":/i18n/i18n/")).entryList();
    translations << QStringLiteral("en_US.qm"); // Equal to no translation
    for(auto &languageCode : translations)
    {
        languageCode.chop(3); // Chop off file extension
        QLocale locale(languageCode);
        QAction *action = new QAction(locale.nativeLanguageName(), ui->menuLanguage);
        connect(action, &QAction::triggered, this, [this,languageCode] { this->switchTranslator(languageCode); });
        ui->menuLanguage->addAction(action);
    }

    //Debugging
    connect(ui->lineEdit, SIGNAL(returnPressed()), this, SLOT(debugCommand()));

    //File transfer
    connect(ui->refreshButton, SIGNAL(clicked(bool)), ui->usblinkTree, SLOT(reloadFilebrowser()));
    connect(ui->usblinkTree, SIGNAL(downloadProgress(int)), this, SLOT(usblinkDownload(int)), Qt::QueuedConnection);
    connect(ui->usblinkTree, SIGNAL(uploadProgress(int)), this, SLOT(changeProgress(int)), Qt::QueuedConnection);
    connect(this, SIGNAL(usblink_progress_changed(int)), this, SLOT(changeProgress(int)), Qt::QueuedConnection);

    // QMLBridge
    KitModel *model = the_qml_bridge->getKitModel();
    connect(model, SIGNAL(anythingChanged()), this, SLOT(kitAnythingChanged()));
    connect(model, SIGNAL(dataChanged(QModelIndex,QModelIndex,QVector<int>)), this, SLOT(kitDataChanged(QModelIndex,QModelIndex,QVector<int>)));
    connect(the_qml_bridge, SIGNAL(currentKitChanged(const Kit&)), this, SLOT(currentKitChanged(const Kit &)));

    //Set up monospace fonts
    QFont monospace = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    monospace.setStyleHint(QFont::Monospace);
    ui->debugConsole->setFont(monospace);
    ui->serialConsole->setFont(monospace);

    //Without this line, Qt prints warning messages...
    qRegisterMetaType<QVector<int>>();

#ifdef Q_OS_ANDROID
    //On android the settings file is deleted everytime you update or uninstall,
    //so choose a better, safer, location
    QString path = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    settings = new QSettings(path + QStringLiteral("/nspire_emu_thread.ini"), QSettings::IniFormat);
#else
    settings = new QSettings();
#endif

    QString prefLang = settings->value(QStringLiteral("preferred_lang"), QStringLiteral("none")).toString();
    if(prefLang != QStringLiteral("none"))
        switchTranslator(QLocale(prefLang));
    else if(appTranslator.load(QLocale::system(), QStringLiteral(":/i18n/i18n/")))
        qApp->installTranslator(&appTranslator);

    updateUIActionState(false);

    //Load settings
    convertTabsToDocks();
    retranslateDocks();
    setExtLCD(settings->value(QStringLiteral("extLCDVisible")).toBool());
    lcd.restoreGeometry(settings->value(QStringLiteral("extLCDGeometry")).toByteArray());
    restoreGeometry(settings->value(QStringLiteral("windowGeometry")).toByteArray());
    restoreState(settings->value(QStringLiteral("windowState")).toByteArray(), WindowStateVersion);

    // Restore dark mode setting
    bool darkModeEnabled = settings->value(QStringLiteral("darkMode"), false).toBool();
    if(darkModeEnabled)
        qputenv("QT_QPA_PLATFORM", QByteArrayLiteral("windows:darkmode=2"));
    else
        qputenv("QT_QPA_PLATFORM", QByteArrayLiteral("windows:darkmode=0"));

#ifdef Q_OS_WIN
    // Windows-only: Create View menu and Dark Mode action
    QMenu *menuView = new QMenu(QStringLiteral("&View"), this);
    QAction *darkModeAction = new QAction(QStringLiteral("&Dark Mode"), this);
    darkModeAction->setCheckable(true);
    darkModeAction->setChecked(savedDarkMode);

    connect(darkModeAction, &QAction::toggled, this, [=](bool checked) {
        if (checked != savedDarkMode) {
            this->settings->setValue("theme/darkMode", checked);
            QMessageBox::information(this, QStringLiteral("Theme Change"), QStringLiteral("Restarting to apply changes..."));
            QProcess::startDetached(QApplication::applicationFilePath(), QApplication::arguments());
            QApplication::quit();
        }
    });

    menuView->addAction(darkModeAction);
    ui->menubar->addMenu(menuView);
#endif

    refillKitMenus();

    ui->lcdView->setFocus();

    // Select default Kit
    bool defaultKitFound = the_qml_bridge->useDefaultKit();

    if(the_qml_bridge->getKitModel()->allKitsEmpty())
    {
        // Do not show the window before MainWindow gets shown,
        // otherwise it won't be in focus.
        QTimer::singleShot(0, this, SIGNAL(openConfiguration()));

        switchUIMode(false);
        show();

        return;
    }

    if(settings->value(QStringLiteral("lastUIMode"), 0).toUInt() == 1)
        switchUIMode(true);
    else
    {
        switchUIMode(false);
        show();
    }

    if(!the_qml_bridge->getAutostart())
    {
        showStatusMsg(tr("Start the emulation via Emulation->Start."));
        return;
    }

    // Autostart handling
    if(!defaultKitFound)
        showStatusMsg(tr("Default Kit not found"));
    else
    {
        bool resumed = false;
        if(!the_qml_bridge->getSnapshotPath().isEmpty())
            resumed = resume();

        if(!resumed)
        {
            // Boot up normally
            if(!emu_thread.boot1.isEmpty() && !emu_thread.flash.isEmpty())
                restart();
            else
                showStatusMsg(tr("Start the emulation via Emulation->Start."));
        }
    }
}

MainWindow::~MainWindow()
{
    if(config_dialog)
        config_dialog->deleteLater();

    if(flash_dialog)
        flash_dialog->deleteLater();

    if(mobileui_component)
        mobileui_component->deleteLater();

    // Save external LCD geometry
    settings->setValue(QStringLiteral("extLCDGeometry"), lcd.saveGeometry());
    settings->setValue(QStringLiteral("extLCDVisible"), lcd.isVisible());

    // Save MainWindow state and geometry
    settings->setValue(QStringLiteral("windowState"), saveState(WindowStateVersion));
    settings->setValue(QStringLiteral("windowGeometry"), saveGeometry());

    delete settings;
    delete ui;
}

void MainWindow::switchTranslator(const QLocale& locale)
{
    qApp->removeTranslator(&appTranslator);
    // For English, nothing to load after removing the translator.
    if (locale.name() == QStringLiteral("en_US")
        || (appTranslator.load(locale, QStringLiteral(":/i18n/i18n/")) && qApp->installTranslator(&appTranslator)))
    {
        settings->setValue(QStringLiteral("preferred_lang"), locale.name());
    }
    else
        QMessageBox::warning(this, tr("Language change"), tr("No translation available for this language :("));
}

void MainWindow::changeEvent(QEvent* event)
{
    const auto eventType = event->type();
    if (eventType == QEvent::LanguageChange)
    {
        ui->retranslateUi(this);
        updateWindowTitle();
        retranslateDocks();
    }
    else if (eventType == QEvent::LocaleChange)
        switchTranslator(QLocale::system());

    QMainWindow::changeEvent(event);
}

void MainWindow::dropEvent(QDropEvent *e)
{
    const QMimeData* mime_data = e->mimeData();
    if(!mime_data->hasUrls())
        return;

    for(auto &&url : mime_data->urls())
    {
        auto local = QDir::toNativeSeparators(url.toLocalFile());
        auto remote = the_qml_bridge->getUSBDir() + QLatin1Char('/') + QFileInfo(local).fileName();
        usblink_queue_put_file(local.toStdString(), remote.toStdString(), usblink_progress_callback, this);
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent *e)
{
    if(e->mimeData()->hasUrls() == false)
        return e->ignore();

    for(QUrl &url : e->mimeData()->urls())
    {
        static const QStringList valid_suffixes = { QStringLiteral("tns"),
                                              QStringLiteral("tno"), QStringLiteral("tnc"),
                                              QStringLiteral("tco"), QStringLiteral("tcc"),
                                              QStringLiteral("tco2"), QStringLiteral("tcc2"),
                                              QStringLiteral("tct2") };

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
    switchUIMode(false);

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
}

void MainWindow::debugCommand()
{
    emit debugStr(QStringLiteral("> %1\n").arg(ui->lineEdit->text()));
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

void MainWindow::switchUIMode(bool mobile_ui)
{
    if(!mobileui_dialog && mobile_ui)
        mobileui_dialog = mobileui_component->create();

    if(mobileui_dialog)
        mobileui_dialog->setProperty("visible", mobile_ui);
    else if(mobile_ui)
    {
        qWarning() << "Could not create mobile UI!";
        return; // Do not switch the UI mode as the mobile UI could not be created
    }

    the_qml_bridge->setActive(mobile_ui);
    this->setActive(!mobile_ui);

    settings->setValue(QStringLiteral("lastUIMode"), mobile_ui ? 1 : 0);
}

void MainWindow::setActive(bool b)
{
    // There is no UniqueQueuedConnection, so we need to avoid duplicate connections
    // manually
    if(b == is_active)
        return;

    is_active = b;

    if(b)
    {
        connect(&emu_thread, SIGNAL(speedChanged(double)), this, SLOT(showSpeed(double)), Qt::QueuedConnection);
        connect(&emu_thread, SIGNAL(turboModeChanged(bool)), ui->buttonSpeed, SLOT(setChecked(bool)), Qt::QueuedConnection);
        connect(&emu_thread, SIGNAL(usblinkChanged(bool)), this, SLOT(usblinkChanged(bool)), Qt::QueuedConnection);
        connect(&emu_thread, SIGNAL(started(bool)), this, SLOT(started(bool)), Qt::QueuedConnection);
        connect(&emu_thread, SIGNAL(paused(bool)), ui->actionPause, SLOT(setChecked(bool)), Qt::QueuedConnection);
        connect(&emu_thread, SIGNAL(resumed(bool)), this, SLOT(resumed(bool)), Qt::QueuedConnection);
        connect(&emu_thread, SIGNAL(suspended(bool)), this, SLOT(suspended(bool)), Qt::QueuedConnection);
        connect(&emu_thread, SIGNAL(stopped()), this, SLOT(stopped()), Qt::QueuedConnection);

        // We might have missed a few events
        updateUIActionState(emu_thread.isRunning());
        ui->buttonSpeed->setChecked(turbo_mode);
        usblinkChanged(usblink_connected);
    }
    else
    {
        disconnect(&emu_thread, SIGNAL(speedChanged(double)), this, SLOT(showSpeed(double)));
        disconnect(&emu_thread, SIGNAL(turboModeChanged(bool)), ui->buttonSpeed, SLOT(setChecked(bool)));
        disconnect(&emu_thread, SIGNAL(usblinkChanged(bool)), this, SLOT(usblinkChanged(bool)));
        disconnect(&emu_thread, SIGNAL(started(bool)), this, SLOT(started(bool)));
        disconnect(&emu_thread, SIGNAL(paused(bool)), ui->actionPause, SLOT(setChecked(bool)));
        disconnect(&emu_thread, SIGNAL(resumed(bool)), this, SLOT(resumed(bool)));
        disconnect(&emu_thread, SIGNAL(suspended(bool)), this, SLOT(suspended(bool)));
        disconnect(&emu_thread, SIGNAL(stopped()), this, SLOT(stopped()));

        // Close the config dialog
        if(config_dialog)
            config_dialog->setProperty("visible", false);
    }

    setVisible(b);
}

void MainWindow::suspendToPath(QString path)
{
    emu_thread.suspend(path);
}

bool MainWindow::resumeFromPath(QString path)
{
    if(!emu_thread.resume(path))
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
    ui->actionLeavePTT->setEnabled(emulation_running);

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

void MainWindow::convertTabsToDocks()
{
    // Create "Docks" menu to make closing and opening docks more intuitive
    QMenu *docks_menu = new QMenu(tr("Docks"), this);
    ui->menubar->insertMenu(ui->menuAbout->menuAction(), docks_menu);

    QAction *editmode_toggle = new QAction(tr("Enable UI edit mode"), this);
    editmode_toggle->setCheckable(true);
    editmode_toggle->setChecked(settings->value(QStringLiteral("uiEditModeEnabled"), true).toBool());
    connect(editmode_toggle, SIGNAL(toggled(bool)), this, SLOT(setUIEditMode(bool)));

    docks_menu->addAction(editmode_toggle);

    docks_menu->addSeparator();

    //Convert the tabs into QDockWidgets
    DockWidget *last_dock = nullptr;
    while(ui->tabWidget->count())
    {
        DockWidget *dw = new DockWidget(ui->tabWidget->tabText(0), this);
        dw->hideTitlebar(true); // Create with hidden titlebar to not resize the window on startup
        dw->setWindowIcon(ui->tabWidget->tabIcon(0));
        // This is used for storing window state, so must not be translated at this point
        dw->setObjectName(ui->tabWidget->tabText(0));

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

    setUIEditMode(editmode_toggle->isChecked());

    ui->tabWidget->setHidden(true);
}

void MainWindow::retranslateDocks()
{
    // The docks are not handled by mainwindow.ui but got created by
    // convertTabsToDocks() above, so translation needs to be done manually.
    const auto dockChildren = findChildren<DockWidget*>();
    for(DockWidget *dw : dockChildren) {
        if(dw->widget() == ui->tab)
            dw->setWindowTitle(tr("Keypad"));
        else if(dw->widget() == ui->tabFiles)
            dw->setWindowTitle(tr("File Transfer"));
        else if(dw->widget() == ui->tabSerial)
            dw->setWindowTitle(tr("Serial Monitor"));
        else if(dw->widget() == ui->tabDebugger)
            dw->setWindowTitle(tr("Debugger"));
    }
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
    /* If there's no kit set, use the default kit */
    if(the_qml_bridge->getCurrentKitId() == -1)
        the_qml_bridge->useDefaultKit();

    applyQMLBridgeSettings();

    auto snapshot_path = the_qml_bridge->getSnapshotPath();
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
    auto snapshot_path = the_qml_bridge->getSnapshotPath();
    if(!snapshot_path.isEmpty())
        suspendToPath(snapshot_path);
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
    if(!flash_dialog)
        flash_dialog = flash_dialog_component->create();

    if(!flash_dialog)
        qWarning() << "Could not create flash dialog!";
    else
        flash_dialog->setProperty("visible", QVariant(true));
}

void MainWindow::setUIEditMode(bool e)
{
    settings->setValue(QStringLiteral("uiEditModeEnabled"), e);

    const auto dockChildren = findChildren<DockWidget*>();
    for(DockWidget *dw : dockChildren)
        dw->hideTitlebar(!e);
}

void MainWindow::showAbout()
{
    aboutDialog.show();
}

void MainWindow::setDarkMode(bool enabled)
{
    settings->setValue(QStringLiteral("darkMode"), enabled);

    if(enabled)
    {
        qputenv("QT_QPA_PLATFORM", QByteArrayLiteral("windows:darkmode=2"));
    }
    else
    {
        qputenv("QT_QPA_PLATFORM", QByteArrayLiteral("windows:darkmode=0"));
    }
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
    if(config_dialog)
        config_dialog->setProperty("visible", false);

    if(flash_dialog)
        flash_dialog->setProperty("visible", false);

    if(!close_after_suspend &&
            settings->value(QStringLiteral("suspendOnClose")).toBool() && emu_thread.isRunning() && exiting == false)
    {
        close_after_suspend = true;
        qDebug("Suspending...");
        suspend();
        e->ignore();
        return;
    }

    if(emu_thread.isRunning() && !emu_thread.stop())
        qDebug("Terminating emulator thread failed.");

    QMainWindow::closeEvent(e);
}

void MainWindow::showStatusMsg(QString str)
{
    status_label.setText(str);
}

void MainWindow::kitDataChanged(QModelIndex, QModelIndex, QVector<int> roles)
{
    if(roles.contains(KitModel::NameRole))
    {
        refillKitMenus();

        // Need to update window title if kit is active
        updateWindowTitle();
    }
}

void MainWindow::kitAnythingChanged()
{
    if(the_qml_bridge->getKitModel()->rowCount() != ui->menuRestart_with_Kit->actions().size())
        refillKitMenus();
}

void MainWindow::currentKitChanged(const Kit &kit)
{
    (void) kit;
    updateWindowTitle();
}

void MainWindow::refillKitMenus()
{
    ui->menuRestart_with_Kit->clear();
    ui->menuBoot_Diags_with_Kit->clear();

    auto &&kit_model = the_qml_bridge->getKitModel();
    for(auto &&kit : kit_model->getKits())
    {
        QAction *action = ui->menuRestart_with_Kit->addAction(kit.name);
        action->setData(kit.id);
        connect(action, SIGNAL(triggered()), this, SLOT(startKit()));

        action = ui->menuBoot_Diags_with_Kit->addAction(kit.name);
        action->setData(kit.id);
        connect(action, SIGNAL(triggered()), this, SLOT(startKitDiags()));
	}
}

void MainWindow::updateWindowTitle()
{
    // Need to update window title if kit is active
    int kitIndex = the_qml_bridge->kitIndexForID(the_qml_bridge->getCurrentKitId());
    if(kitIndex >= 0)
    {
        auto name = the_qml_bridge->getKitModel()->getKits()[kitIndex].name;
        setWindowTitle(tr("Firebird Emu - %1").arg(name));
    }
    else
        setWindowTitle(tr("Firebird Emu"));
}

void MainWindow::applyQMLBridgeSettings()
{
    // Reload the current kit
    the_qml_bridge->useKit(the_qml_bridge->getCurrentKitId());

    emu_thread.port_gdb = the_qml_bridge->getGDBEnabled() ? the_qml_bridge->getGDBPort() : 0;
    emu_thread.port_rdbg = the_qml_bridge->getRDBEnabled() ? the_qml_bridge->getRDBPort() : 0;
}

void MainWindow::restart()
{
    /* If there's no kit set, use the default kit */
    if(the_qml_bridge->getCurrentKitId() == -1)
        the_qml_bridge->useDefaultKit();

    applyQMLBridgeSettings();

    if(emu_thread.boot1.isEmpty())
    {
        QMessageBox::critical(this, tr("No boot1 set"), tr("Before you can start the emulation, you have to select a proper boot1 file."));
        return;
    }

    if(emu_thread.flash.isEmpty())
    {
        QMessageBox::critical(this, tr("No flash image loaded"), tr("Before you can start the emulation, you have to load a proper flash file.\n"
                                                                            "You can create one via Flash->Create Flash in the menu."));
        return;
    }

    if(emu_thread.stop())
        emu_thread.start();
    else
        QMessageBox::warning(this, tr("Restart needed"), tr("Failed to restart emulator. Close and reopen this app.\n"));
}

void MainWindow::openConfiguration()
{
    if(!config_dialog)
        config_dialog = config_component->create();

    if(!config_dialog)
        qWarning() << "Could not create config dialog!";
    else
        config_dialog->setProperty("visible", QVariant(true));
}

void MainWindow::startKit()
{
    auto action = qobject_cast<QAction*>(sender());
    if(!action)
    {
        qWarning() << "Received signal from invalid sender";
        return;
    }

    auto kit_id = static_cast<unsigned int>(action->data().toInt());
    the_qml_bridge->setCurrentKit(kit_id);
    boot_order = ORDER_BOOT2;
    restart();
}

void MainWindow::startKitDiags()
{
    auto action = qobject_cast<QAction*>(sender());
    if(!action)
    {
        qWarning() << "Received signal from invalid sender";
        return;
    }

    auto kit_id = static_cast<unsigned int>(action->data().toInt());
    the_qml_bridge->setCurrentKit(kit_id);
    boot_order = ORDER_DIAGS;
    restart();
}

void MainWindow::xmodemSend()
{
    QString filename = QFileDialog::getOpenFileName(this, tr("Select file to send"));
    if(filename.isEmpty())
        return;

    std::string path = filename.toStdString();
    xmodem_send(path.c_str());
}

void MainWindow::switchToMobileUI()
{
    switchUIMode(true);
}

bool QQuickWidgetLessBroken::event(QEvent *event)
{
    if(event->type() == QEvent::Leave)
    {
        QMouseEvent ev(QMouseEvent::MouseMove, QPointF(0, 0), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
        QQuickWidget::event(&ev);
    }

    return QQuickWidget::event(event);
}
