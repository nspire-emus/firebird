#ifndef APPTRANSLATION_H
#define APPTRANSLATION_H

#include <QLocale>
#include <QTranslator>

inline bool loadAppTranslation(QTranslator &translator, const QLocale &locale)
{
    const QString path = QStringLiteral(":/i18n/i18n/");

    // Qt's locale lookup does not fall back across regions. Select the
    // available Chinese catalog by script, including for Hong Kong,
    // Macau and Singapore, and honor an explicitly requested script.
    if(locale.language() == QLocale::Chinese)
    {
        const QString catalog = locale.script() == QLocale::TraditionalHanScript
                ? QStringLiteral("zh_TW") : QStringLiteral("zh_CN");
        return translator.load(catalog, path);
    }

    return translator.load(locale, path);
}

#endif
