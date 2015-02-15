#ifndef FLASHDIALOG_H
#define FLASHDIALOG_H

#include <QDialog>

namespace Ui {
class FlashDialog;
}

class FlashDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FlashDialog(QWidget *parent = 0);
    ~FlashDialog();

public slots:
    void selectBoot2();
    void selectManuf();
    void selectOS();
    void saveAs();

private:
    Ui::FlashDialog *ui;

    QString boot2_path, manuf_path, os_path;
};

#endif // FLASHDIALOG_H
