#include "about.h"
#include <QLabel>
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include <QtCore>
#include <QScrollBar>
#include "globalobjects.h"
About::About(QWidget *parent) : CFramelessDialog("",parent)
{
    QLabel *logo=new QLabel(this);
    logo->setPixmap(QPixmap(":/res/images/kikoplay-4.png"));
    logo->setAlignment(Qt::AlignCenter);
    QLabel *info=new QLabel(tr("KikoPlay - NOT ONLY A Full-Featured Danmu Player<br/>"
                               "%1 (C) 2023 Kikyou <a style='color: rgb(96, 208, 252);' href=\"https://kikoplayproject.github.io/\">homepage</a> <a style='color: rgb(96, 208, 252);' href=\"https://github.com/KikoPlayProject/KikoPlay\">github</a><br/>"
                               "Exchange & BUG Report: 874761809(QQ Group)").arg(GlobalObjects::kikoVersion),this);
    info->setOpenExternalLinks(true);
    info->setAlignment(Qt::AlignCenter);
    QPlainTextEdit *libsInfo=new QPlainTextEdit(this);
    libsInfo->setReadOnly(true);
    QFile aboutFile(":/res/about");
    aboutFile.open(QFile::ReadOnly);
    if(aboutFile.isOpen())
    {
        libsInfo->appendPlainText(aboutFile.readAll());
    }
    QTextCursor cursor = libsInfo->textCursor();
    cursor.movePosition(QTextCursor::Start);
    libsInfo->setTextCursor(cursor);
    QVBoxLayout *abVLayout=new QVBoxLayout(this);
    abVLayout->addWidget(logo);
    abVLayout->addWidget(info);
    abVLayout->addWidget(libsInfo);
    setResizeable(false);
}
