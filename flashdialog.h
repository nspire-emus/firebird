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
    void selectDiags();
    void saveAs();

signals:
    void flashCreated(QString path);

private:
    QString readVersion(QString file);

    Ui::FlashDialog *ui;

    QString boot2_path, manuf_path, diags_path, os_path;
};

#endif // FLASHDIALOG_H
