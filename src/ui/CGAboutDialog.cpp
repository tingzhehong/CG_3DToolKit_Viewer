#include "CGAboutDialog.h"
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>

CGAboutDialog::CGAboutDialog(QWidget *parent): QDialog(parent)
{
    InitUi();
    InitConnections();
}

void CGAboutDialog::OnOK()
{
    this->accept();
}

void CGAboutDialog::InitUi()
{
    setWindowTitle(tr(u8"关于"));
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    setWindowIcon(QIcon(":/icon/logo.png"));
    resize(300, 200);

    m_pAboutLb = new QLabel(tr(u8"CG_3DToolKit Viewer"), this);
    m_pVersion = new QLabel(tr(u8"版本：Ver2205 FE"), this);
    m_pOKBtn = new QPushButton(tr(u8"确定"), this);
    m_pOKBtn->setIcon(QIcon(":/icon/logo.png"));

    QHBoxLayout *pFristLayout = new QHBoxLayout();
    pFristLayout->addStretch();
    pFristLayout->addWidget(m_pAboutLb);
    pFristLayout->addStretch();

    QHBoxLayout *pSecondLayout = new QHBoxLayout();
    pSecondLayout->addStretch();
    pSecondLayout->addWidget(m_pVersion);
    pSecondLayout->addStretch();

    QHBoxLayout *pLastLayout = new QHBoxLayout();
    pLastLayout->addStretch();
    pLastLayout->addWidget(m_pOKBtn);
    pLastLayout->addStretch();

    QVBoxLayout *pMainLayout = new QVBoxLayout();
    pMainLayout->addStretch();
    pMainLayout->addLayout(pFristLayout);
    pMainLayout->addLayout(pSecondLayout);
    pMainLayout->addStretch();
    pMainLayout->addLayout(pLastLayout);
    pMainLayout->addStretch();

    setLayout(pMainLayout);
}

void CGAboutDialog::InitConnections()
{
    connect(m_pOKBtn, &QPushButton::clicked, this, &CGAboutDialog::OnOK);
}
