#include "CGConsoleView.h"
#include <QFont>
#include <QLabel>
#include <QMessageBox>
#include <QDateTime>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>

CGConsoleView *CGConsoleView::m_CGConsoleView = nullptr;

CGConsoleView::CGConsoleView(QWidget *parent) : QWidget(parent)
{
    InitUi();
    InitConnections();
}

CGConsoleView *CGConsoleView::getInstance()
{
    if (!m_CGConsoleView)
    {
        m_CGConsoleView = new CGConsoleView();
    }
    return m_CGConsoleView;
}

void CGConsoleView::InitUi()
{
    QFont font;
    font.setBold(true);
    m_pListWidget = new QListWidget(this);
    m_pListWidget->setStyleSheet("background-color:rgb(220, 220, 220)");

    QVBoxLayout *pMainLayout = new QVBoxLayout();
    pMainLayout->addWidget(m_pListWidget);

    setLayout(pMainLayout);
    setFixedHeight(120);
    setVisible(false);
}

void CGConsoleView::InitConnections()
{

}

void CGConsoleView::ConsoleOut(const QString msg)
{
    QDateTime CurrentDataTime = QDateTime::currentDateTime();
    QString current_date_time = "[" + CurrentDataTime.toString("yyyy/MM/dd hh:mm:ss:zzz") + "]";
    QString str;
    str.append(current_date_time);
    str.append(" : ");
    str.append(msg);

    QListWidgetItem *item = new QListWidgetItem(str);
    m_pListWidget->insertItem(0, item);

    int num = m_pListWidget->count();
    if (num >= 12) {
        m_pListWidget->clear();
    }
}
