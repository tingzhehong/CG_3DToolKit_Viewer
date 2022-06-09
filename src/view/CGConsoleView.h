#ifndef CGCONSOLEVIEW_H
#define CGCONSOLEVIEW_H

#include <QWidget>
#include <QListWidget>

class CGConsoleView : public QWidget
{
    Q_OBJECT

private:
    explicit CGConsoleView(QWidget *parent = nullptr);

signals:

public:
    static CGConsoleView *getInstance();
    void InitUi();
    void InitConnections();
    void ConsoleOut(const QString msg);

    QListWidget *m_pListWidget;

private:
    static CGConsoleView *m_CGConsoleView;

};

#endif // CGCONSOLEVIEW_H
