#ifndef DOCKWIDGET_H
#define DOCKWIDGET_H

#include <QDockWidget>

/* This class augments QDockWidget with the function
 * to hide the titlebar of non-floating docks. */

class DockWidget : public QDockWidget
{
    Q_OBJECT
public:
    explicit DockWidget(const QString &title, QWidget *parent = nullptr,
                         Qt::WindowFlags flags = Qt::WindowFlags());

    explicit DockWidget(QWidget *parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags())
        : DockWidget({}, parent, flags) {}

    ~DockWidget() {}

    void hideTitlebar(bool b);

protected slots:
    void refreshTitlebar();

protected:
    bool hide_titlebar_if_possible = false;
    QWidget empty_titlebar;
};

#endif // DOCKWIDGET_H
