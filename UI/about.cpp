#include "about.h"
#include <QLabel>
#include <QPlainTextEdit>
#include <QVBoxLayout>
About::About(QWidget *parent) : CFramelessDialog("",parent)
{
    QLabel *logo=new QLabel(this);
    logo->setPixmap(QPixmap(":/res/images/kikoplay-4.png"));
    logo->setAlignment(Qt::AlignCenter);
    QLabel *info=new QLabel(tr("KikoPlay - A Full-featured Danmu Player<br/>"
							   "(C) 2018 Kikyou <a href=\"https://github.com/Protostars/KikoPlay\">github</a><br/>"
							   "Exchange & BUG Report: 874761809(QQ Group)<br/>"
                               "KikoPlay is based on the following projects:"),this);
    info->setOpenExternalLinks(true);
    info->setAlignment(Qt::AlignCenter);
    QPlainTextEdit *libsInfo=new QPlainTextEdit(this);
    libsInfo->setReadOnly(true);
    libsInfo->appendPlainText("Qt 5.10.1\n"
                              "libmpv 20180902\n"
                              "aria2 1.34\n"
                              "Qt-Nice-Frameless-Window\n"
                              "zlib 1.2.11");
    QVBoxLayout *abVLayout=new QVBoxLayout(this);
    abVLayout->addWidget(logo);
    abVLayout->addWidget(info);
    abVLayout->addWidget(libsInfo);
    setResizeable(false);
}
