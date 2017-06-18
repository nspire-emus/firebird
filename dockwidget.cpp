#include "dockwidget.h"

DockWidget::DockWidget(const QString &title, QWidget *parent, Qt::WindowFlags flags)
    : QDockWidget(title, parent, flags)
{
    connect(this, SIGNAL(topLevelChanged(bool)), this, SLOT(refreshTitlebar()));
}

void DockWidget::hideTitlebar(bool b)
{
    hide_titlebar_if_possible = b;
    refreshTitlebar();
}

void DockWidget::refreshTitlebar()
{
    if(hide_titlebar_if_possible && !isFloating())
        setTitleBarWidget(&empty_titlebar);
    else
        setTitleBarWidget(nullptr); // Restore default titlebar
}
