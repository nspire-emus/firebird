#include <QTextStream>
#include <QFileDialog>
#include <QMessageBox>

#include "core/emu.h"
#include "core/flash.h"
#include "flashdialog.h"
#include "ui_flashdialog.h"

FlashDialog::FlashDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FlashDialog)
{
    ui->setupUi(this);

    connect(ui->buttonBoot2, SIGNAL(clicked()), this, SLOT(selectBoot2()));
    connect(ui->buttonManuf, SIGNAL(clicked()), this, SLOT(selectManuf()));
    connect(ui->buttonOS, SIGNAL(clicked()), this, SLOT(selectOS()));
    connect(ui->buttonDiags, SIGNAL(clicked(bool)), this, SLOT(selectDiags()));
    connect(ui->buttonSave, SIGNAL(clicked()), this, SLOT(saveAs()));

    connect(ui->selectModel, SIGNAL(currentIndexChanged(int)), this, SLOT(hwTypeChanged(int)));
}

FlashDialog::~FlashDialog()
{
    delete ui;
}

QString FlashDialog::readVersion(QString path)
{
    //Ugly way to display version
    QFile file(path);
    QByteArray header;
    if(!file.open(QFile::ReadOnly) || !file.seek(31) || (header = file.read(17)).isEmpty())
        return tr("Unknown");

    unsigned int len = header.at(0);
    if(len < 16 && header.at(len + 1) == static_cast<char>(0x80))
        return QString::fromUtf8(header.data() + 1, len);

    return tr("Unknown");
}

void FlashDialog::selectBoot2()
{
    QString path = QFileDialog::getOpenFileName(this, tr("Select Boot2"));
    if(path.isEmpty() || !QFile(path).exists())
    {
        boot2_path = QString();
        ui->labelBoot2->setText(tr("None"));
        return;
    }

    boot2_path = path;
    ui->labelBoot2->setText(readVersion(boot2_path));
}

void FlashDialog::selectManuf()
{
    QString path = QFileDialog::getOpenFileName(this, tr("Select Manuf"));
    if(path.isEmpty() || !QFile(path).exists())
    {
        manuf_path = QString();
        ui->labelManuf->setText(tr("None"));
        return;
    }

    manuf_path = path;

    ui->labelManuf->setText(tr("Loaded"));
}

// Map of ui->selectModel indices to OS filename extensions
const QString os_ext[] = { QStringLiteral("*.tno"), QStringLiteral("*.tnc"), QStringLiteral("*.tco"), QStringLiteral("*.tcc"),  QStringLiteral("*.tco2 *.tcc2 *.tct2")};

void FlashDialog::selectOS()
{
    QString path = QFileDialog::getOpenFileName(this, tr("Select OS file"), QString(), tr("OS file (%1)").arg(os_ext[ui->selectModel->currentIndex()]));
    if(path.isEmpty() || !QFile(path).exists())
    {
        os_path = QString();
        ui->labelOS->setText(tr("None"));
        return;
    }

    os_path = path;

    ui->labelOS->setText(tr("Unknown"));

    //Ugly way to display version
    QFile os(path);
    QString header;
    if(!os.open(QFile::ReadOnly) || (header = QString::fromUtf8(os.read(128))).isEmpty())
        return;

    QString filename, version;
    QTextStream ts(&header);
    ts >> filename >> version;

    if(filename.endsWith(QStringLiteral(".tnc")))
        version += QStringLiteral(" CAS");
    else if(filename.endsWith(QStringLiteral(".tco")))
        version += QStringLiteral(" CX");
    else if(filename.endsWith(QStringLiteral(".tcc")))
        version += QStringLiteral(" CX CAS");
    else if(filename.endsWith(QStringLiteral(".tco2")))
        version += QStringLiteral(" CX II");
    else if(filename.endsWith(QStringLiteral(".tcc2")))
        version += QStringLiteral(" CX II CAS");
    else if(filename.endsWith(QStringLiteral(".tct2")))
        version += QStringLiteral(" CX II-T");

    ui->labelOS->setText(version);
}

void FlashDialog::selectDiags()
{
    QString path = QFileDialog::getOpenFileName(this, tr("Select Diags"));
    if(path.isEmpty() || !QFile(path).exists())
    {
        diags_path = QString();
        ui->labelDiags->setText(tr("None"));
        return;
    }

    diags_path = path;
    ui->labelDiags->setText(readVersion(diags_path));
}

void FlashDialog::hwTypeChanged(int i)
{
    // HW-rev can only be selected for CX (CAS)
    ui->selectModelRev->setEnabled(i == 2 || i == 3);
}

// Map of ui->selectModel indices to manuf product numbers
const unsigned int product_values[] = { 0x0E0, 0x0C2, 0x100, 0x0F0, 0x1C0 };

// Map of ui->selectModelRev indices to manuf feature values
const unsigned int feature_values[] = { FEATURE_CX, FEATURE_HWJ, FEATURE_HWW };

void FlashDialog::saveAs()
{
    QString path = QFileDialog::getSaveFileName(this, tr("Save flash image"));
    if(path.isEmpty())
        return;

    bool is_cx = ui->selectModel->currentIndex() >= 2;
    std::string preload_str[4] = { manuf_path.toStdString(), boot2_path.toStdString(), diags_path.toStdString(), os_path.toStdString() };
    const char *preload[4] = { nullptr, nullptr, nullptr, nullptr };

    for(unsigned int i = 0; i < 4; ++i)
        if(preload_str[i] != "")
            preload[i] = preload_str[i].c_str();

    uint8_t *nand_data;
    size_t nand_size;

    if(!flash_create_new(is_cx, preload, product_values[ui->selectModel->currentIndex()], feature_values[ui->selectModelRev->currentIndex()], is_cx, &nand_data, &nand_size))
    {
        QMessageBox::critical(this, tr("Flash creation failed"), tr("Creating the flash file failed!"));

        free(nand_data);

        return;
    }

    QFile flash_file(path);
    if(!flash_file.open(QFile::WriteOnly) || !flash_file.write(reinterpret_cast<char*>(nand_data), nand_size))
    {
        QMessageBox::critical(this, tr("Flash saving failed"), tr("Saving the flash file failed!"));

        free(nand_data);

        return;
    }

    free(nand_data);

    flash_file.close();

    close();

    emit flashCreated(path);
}
