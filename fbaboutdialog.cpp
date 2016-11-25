#include "fbaboutdialog.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>

#include <QIcon>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QPushButton>
#include <QTimer>

#define STRINGIFYMAGIC(x) #x
#define STRINGIFY(x) STRINGIFYMAGIC(x)

FBAboutDialog::FBAboutDialog(QWidget *parent)
    : QDialog(parent)
{
    QIcon icon{QStringLiteral(":/icons/resources/firebird.png")};
    iconLabel.setPixmap(icon.pixmap(icon.actualSize(QSize{64, 64})));

    setWindowTitle(tr("About Firebird"));
    header.setText(tr("<h3>Firebird %1</h3>"
                      "<a href='https://github.com/nspire-emus/firebird'>On GitHub</a>").arg(QStringLiteral(STRINGIFY(FB_VERSION))));
    header.setTextInteractionFlags(Qt::TextBrowserInteraction);
    header.setOpenExternalLinks(true);

    update.setText(tr("Checking for update"));
    update.setTextInteractionFlags(Qt::TextBrowserInteraction);
    update.setOpenExternalLinks(true);

    authors.setText(tr(  "Authors:<br>"
                         "Fabian Vogt (<a href='https://github.com/Vogtinator'>Vogtinator</a>)<br>"
                         "Adrien Bertrand (<a href='https://github.com/adriweb'>Adriweb</a>)<br>"
                         "Antonio Vasquez (<a href='https://github.com/antoniovazquezblanco'>antoniovazquezblanco</a>)<br>"
                         "Lionel Debroux (<a href='https://github.com/debrouxl'>debrouxl</a>)<br>"
                         "Denis Avashurov (<a href='https://github.com/denisps'>denisps</a>)<br>"
                         "Based on nspire_emu v0.70 by Goplat<br><br>"
                         "This work is licensed under the GPLv3.<br>"
                         "To view a copy of this license, visit <a href='https://www.gnu.org/licenses/gpl-3.0.html'>https://www.gnu.org/licenses/gpl-3.0.html</a>"));
    authors.setTextInteractionFlags(Qt::TextBrowserInteraction);
    authors.setOpenExternalLinks(true);

    auto *okButton = new QPushButton(tr("Ok"));
    connect(okButton, SIGNAL(clicked(bool)), this, SLOT(close()));
    okButton->setDefault(true);

    updateButton.setText(tr("Check for Update"));
    connect(&updateButton, SIGNAL(clicked(bool)), this, SLOT(checkForUpdate()));

    auto *buttonBox = new QDialogButtonBox(Qt::Horizontal);
    buttonBox->addButton(okButton, QDialogButtonBox::AcceptRole);
    buttonBox->addButton(&updateButton, QDialogButtonBox::ActionRole);

    auto *layout = new QVBoxLayout;
    layout->addWidget(&header);
    layout->addWidget(&update);
    layout->addWidget(&authors);
    layout->addWidget(buttonBox);

    auto *hlayout = new QHBoxLayout(this);
    hlayout->addWidget(&iconLabel);
    hlayout->addLayout(layout);
}

void FBAboutDialog::checkForUpdate()
{
    updateButton.setDisabled(true);

    if(QStringLiteral(STRINGIFY(FB_VERSION)).contains(QStringLiteral("dev")))
    {
            update.setText(tr("No updates for -dev builds available."));
            return;
    }

    checkSuccessful = false;
    update.setText(tr("Checking for updates..."));

    QNetworkRequest request(QUrl(QString::fromLatin1("https://api.github.com/repos/nspire-emus/firebird/releases/latest")));

    reply = nam.get(request);

    connect(reply, SIGNAL(finished()), this, SLOT(requestFinished()));
}

void FBAboutDialog::requestFinished()
{
    reply->deleteLater();
    updateButton.setDisabled(false);

    /* toInt() returns 0 if conversion fails. That fits nicely already. */
    int code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    auto response = QJsonDocument::fromJson(reply->readAll());

    if(code != 200 || response.isEmpty())
    {
        update.setText(tr("Checking failed (%1)").arg(reply->errorString()));
        return;
    }

    QString tag_name = response.object()[QLatin1String("tag_name")].toString(),
            url = response.object()[QLatin1String("html_url")].toString(),
            title = response.object()[QLatin1String("name")].toString();

    if(tag_name == QStringLiteral("v" STRINGIFY(FB_VERSION)))
    {
        update.setText(tr("No newer version available."));
        checkSuccessful = true;
    }
    else if(tag_name.at(0) == QLatin1Char('v'))
    {
        update.setText(tr("<b>Newer version (%1) available <a href='%2'>on GitHub</a>!</b>").arg(title.toHtmlEscaped()).arg(url.toHtmlEscaped()));
        checkSuccessful = true;
    }
    else
        update.setText(tr("Checking failed (invalid tag name)"));
}

void FBAboutDialog::setVisible(bool v)
{
    QDialog::setVisible(v);

    if(v && !checkSuccessful)
        QTimer::singleShot(0, this, SLOT(checkForUpdate()));
}
