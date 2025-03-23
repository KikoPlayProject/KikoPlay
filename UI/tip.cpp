#include "tip.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QFile>
#include <QApplication>
Tip::Tip(QWidget *parent) : CFramelessDialog (tr("Tip"),parent)
{
    QLabel *tipContent=new QLabel(this);
    tipContent->setTextInteractionFlags(Qt::TextBrowserInteraction);
    tipContent->setWordWrap(true);
    tipContent->setOpenExternalLinks(true);
    QFile tipFile(":/res/tip");
    tipFile.open(QFile::ReadOnly);
    if(tipFile.isOpen())
    {
        QString tipText = tipFile.readAll();
        tipText = tipText.replace("{AppPath}", QCoreApplication::applicationDirPath());
        tipContent->setText(tipText);
    }
    tipContent->adjustSize();
    QVBoxLayout *tipVLayout=new QVBoxLayout(this);
    tipVLayout->addWidget(tipContent);
    tipVLayout->addStretch(1);
}
