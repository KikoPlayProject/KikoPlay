#include "about.h"
#include <QLabel>
#include <QVBoxLayout>
About::About(QWidget *parent) : CFramelessDialog("",parent)
{
    QLabel *logo=new QLabel(this);
    logo->setPixmap(QPixmap(":/res/images/kikoplay-4.png"));
    logo->setAlignment(Qt::AlignCenter);
    QLabel *info=new QLabel(tr("KikoPlay - A Full-featured Danmu Player<br/>"
							   "(C) 2018 Kikyou <a href=\"https://github.com/Protostars/KikoPlay\">github</a><br/>"
							   "Exchange & BUG Report: 874761809(QQ Group)<br/>"
                               "KikoPlay is based on the following projects:<br/>"
                               "Qt 5.10.1<br/>"
                               "libmpv 20180902<br/>"
                               "aria2 1.34<br/>"
                               "Qt-Nice-Frameless-Window"),this);
    info->setOpenExternalLinks(true);
    info->setAlignment(Qt::AlignCenter);
    QVBoxLayout *abVLayout=new QVBoxLayout(this);
    abVLayout->addWidget(logo);
    abVLayout->addWidget(info);
    setResizeable(false);
}
