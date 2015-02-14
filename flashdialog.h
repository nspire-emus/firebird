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

private:
    Ui::FlashDialog *ui;
};

#endif // FLASHDIALOG_H
