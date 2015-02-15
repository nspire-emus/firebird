#include "flashdialog.h"
#include "ui_flashdialog.h"

#include <QTextStream>
#include <QFileDialog>
#include <QMessageBox>

FlashDialog::FlashDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FlashDialog)
{
    ui->setupUi(this);

    connect(ui->buttonBoot2, SIGNAL(clicked()), this, SLOT(selectBoot2()));
    connect(ui->buttonManuf, SIGNAL(clicked()), this, SLOT(selectManuf()));
    connect(ui->buttonOS, SIGNAL(clicked()), this, SLOT(selectOS()));
    connect(ui->buttonSave, SIGNAL(clicked()), this, SLOT(saveAs()));
}

FlashDialog::~FlashDialog()
{
    delete ui;
}

void FlashDialog::selectBoot2()
{
    QString path = QFileDialog::getOpenFileName(this, trUtf8("Select Boot2"));
    if(path.isEmpty() || !QFile(path).exists())
    {
        boot2_path = "";
        ui->labelBoot2->setText("None");
        return;
    }

    ui->labelBoot2->setText("Unknown");

    //Ugly way to display version
    QFile boot2(path);
    QByteArray header;
    if(!boot2.open(QFile::ReadOnly) || !boot2.seek(32) || (header = boot2.read(16)).isEmpty())
        return;

    if(header.at(7 == 0x80))
        ui->labelBoot2->setText(QString::fromUtf8(header.data(), 6));
}

void FlashDialog::selectManuf()
{
    QString path = QFileDialog::getOpenFileName(this, trUtf8("Select Manuf"));
    if(path.isEmpty() || !QFile(path).exists())
    {
        manuf_path = "";
        ui->labelManuf->setText("None");
        return;
    }

    ui->labelManuf->setText("OK");
}

// Map of ui->selectModel indices to OS filename extensions
const QString os_ext[] = { "*.tno", "*.tnc", "*.tco", "*.tcc" };

void FlashDialog::selectOS()
{
    QString path = QFileDialog::getOpenFileName(this, trUtf8("Select OS file"), QString(), os_ext[ui->selectModel->currentIndex()]);
    if(!path.isEmpty() && QFile(path).exists())
        os_path = path;

    ui->labelOS->setText("Unknown");

    //Ugly way to display version
    QFile os(path);
    QString header;
    if(!os.open(QFile::ReadOnly) || (header = QString::fromUtf8(os.read(128))).isEmpty())
        return;

    QString filename, version;
    QTextStream ts(&header);
    ts >> filename >> version;

    if(filename.endsWith(".tnc"))
        version += " CAS";
    else if(filename.endsWith(".tco"))
        version += " CX";
    else if(filename.endsWith(".tcc"))
        version += " CX CAS";

    ui->labelOS->setText(version);
}

extern "C" {
    #include "flash.h"
}

// Map of ui->selectModel indices to manuf product numbers
const unsigned int product_values[] = { 0x0E0, 0x0C1, 0x100, 0x0F0 };

void FlashDialog::saveAs()
{
    QString path = QFileDialog::getSaveFileName(this, trUtf8("Save flash image"));
    if(path.isEmpty())
        return;

    bool is_cx = ui->selectModel->currentIndex() >= 2;
    std::string preload_str[4] = { manuf_path.toStdString(), boot2_path.toStdString(), "", os_path.toStdString() };
    const char *preload[4] = { nullptr, nullptr, nullptr, nullptr };

    for(unsigned int i = 0; i < 4; ++i)
        if(preload_str[i] != "")
            preload[i] = preload_str[i].c_str();

    uint8_t *nand_data;
    size_t nand_size;

    if(!flash_create_new(is_cx, preload, product_values[ui->selectModel->currentIndex()], is_cx, &nand_data, &nand_size))
    {
        QMessageBox::critical(this, QString("Flash creation failed"), QString("Creating the flash file failed!"));
        return;
    }

    QFile flash_file(path);
    if(!flash_file.open(QFile::WriteOnly) || !flash_file.write(reinterpret_cast<char*>(nand_data), nand_size))
    {
        QMessageBox::critical(this, QString("Flash saving failed"), QString("Saving the flash file failed!"));

        free(nand_data);

        return;
    }

    free(nand_data);

    close();
}
