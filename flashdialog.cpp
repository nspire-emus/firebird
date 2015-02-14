#include "flashdialog.h"
#include "ui_flashdialog.h"

FlashDialog::FlashDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FlashDialog)
{
    ui->setupUi(this);
}

FlashDialog::~FlashDialog()
{
    delete ui;
}
