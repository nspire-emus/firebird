#ifndef FBABOUTDIALOG_H
#define FBABOUTDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QNetworkReply>

class FBAboutDialog : public QDialog
{
Q_OBJECT

public:
    FBAboutDialog(QWidget *parent);

public slots:
    void checkForUpdate();
    void requestFinished();

    void setVisible(bool v) override;

protected:
    void changeEvent(QEvent* event) override;

private:
    void retranslateUi();

    bool checkSuccessful = false;

    QLabel iconLabel, header, update, authors;
    QPushButton okButton, updateButton;
    QNetworkReply *reply;
    QNetworkAccessManager nam;
};

#endif // FBABOUTDIALOG_H
