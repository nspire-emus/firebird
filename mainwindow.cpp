#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QTextBlock>

MainWindow *main_window;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    emu_thread = &emu;

    ui->setupUi(this);

    connect(&refresh_timer, SIGNAL(timeout()), this, SLOT(refresh()));

    //Emu -> GUI
    connect(&emu, SIGNAL(serialChar(char)), this, SLOT(serialChar(char)), Qt::QueuedConnection);
    connect(&emu, SIGNAL(debugStr(QString)), this, SLOT(debugStr(QString)), Qt::QueuedConnection);
    connect(&emu, SIGNAL(statusMsg(QString)), ui->statusbar, SLOT(showMessage(QString)), Qt::QueuedConnection);

    //Menu
    connect(ui->actionReset, SIGNAL(triggered()), &emu, SLOT(reset()));
    connect(ui->actionRestart, SIGNAL(triggered()), this, SLOT(restart()));
    connect(ui->actionDebugger, SIGNAL(triggered()), &emu, SLOT(enterDebugger()));
    connect(ui->actionPause, SIGNAL(toggled(bool)), &emu, SLOT(setPaused(bool)));

    //Debugging
    connect(ui->lineEdit, SIGNAL(returnPressed()), this, SLOT(debugCommand()));

    //Settings
    connect(ui->fileBoot1, SIGNAL(pressed()), this, SLOT(selectBoot1()));
    connect(ui->fileFlash, SIGNAL(pressed()), this, SLOT(selectFlash()));

    refresh_timer.setInterval(1000 / 60); //60 fps
    refresh_timer.start();

    ui->lcdView->setScene(&lcd_scene);

    //Load settings
    selectBoot1(settings.value("boot1", "").toString());
    selectFlash(settings.value("flash", "").toString());

    if(emu.emu_path_boot1 != "" && emu.emu_path_flash != "")
        emu.start();
}

MainWindow::~MainWindow()
{
    delete ui;
}

extern "C"
{
    #include "lcd.h"
}

void MainWindow::refresh()
{
    lcd_scene.clear();

    QByteArray framebuffer(320 * 240 * 2, 0);
    uint32_t bitfields[3];
    lcd_cx_draw_frame(reinterpret_cast<uint16_t*>(framebuffer.data()), bitfields);
    QImage::Format format = bitfields[0] == 0x00F ? QImage::Format_RGB444 : QImage::Format_RGB16;
    if(!emulate_cx)
    {
        format = QImage::Format_RGB444;
        uint16_t *px = reinterpret_cast<uint16_t*>(framebuffer.data());
        for(unsigned int i = 0; i < 320*240; ++i)
        {
            uint16_t n = *px << 8 | *px << 4 | *px;
            *px = ~n;
            ++px;
        }
    }
    QImage image(reinterpret_cast<const uchar*>(framebuffer.data()), 320, 240, 320 * 2, format);

    lcd_scene.addPixmap(QPixmap::fromImage(image));
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

void MainWindow::debugStr(QString str)
{
    ui->debugConsole->moveCursor(QTextCursor::End);
    ui->debugConsole->insertPlainText(str);

    ui->tabWidget->setCurrentWidget(ui->tabDebugger);
}

void MainWindow::debugCommand()
{
    debug_command = ui->lineEdit->text().toLatin1();
    emit debuggerCommand();
}

void MainWindow::selectBoot1(QString path)
{
    QFileInfo f(path);
    emu.emu_path_boot1 = path.toStdString();
    ui->filenameBoot1->setText(f.fileName());

    settings.setValue("boot1", path);
}

void MainWindow::selectBoot1()
{
    QFileInfo f(QString::fromStdString(emu.emu_path_flash));
    QString path = QFileDialog::getOpenFileName(this, "Select boot1 file", f.dir().absolutePath());
    if(!path.isNull())
        selectBoot1(path);
}

void MainWindow::selectFlash(QString path)
{
    QFileInfo f(path);
    emu.emu_path_flash = path.toStdString();
    ui->filenameFlash->setText(f.fileName());

    settings.setValue("flash", path);
}

void MainWindow::selectFlash()
{
    QFileInfo f(QString::fromStdString(emu.emu_path_flash));
    QString path = QFileDialog::getOpenFileName(this, "Select flash file", f.dir().absolutePath());
    if(!path.isNull())
        selectFlash(path);
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    qDebug("Terminating emulator thread...");

    if(emu.stop())
        qDebug("Successful!");
    else
        qDebug("Failed.");

    QMainWindow::closeEvent(e);
}

void MainWindow::restart()
{
    if(emu.stop())
        emu.start();
    else
        debugStr("Failed to restart emulator. Close and reopen this app.");
}
