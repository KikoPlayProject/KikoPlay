#include "mediainfo.h"
#include <QTextEdit>
#include <QHBoxLayout>
#include "globalobjects.h"
#include "Play/Video/mpvplayer.h"

MediaInfo::MediaInfo(QWidget *parent) : CFramelessDialog(tr("Media Info"),parent)
{
    QTextEdit *infoText=new QTextEdit(this);
    QMap<QString,QMap<QString,QString> > info=GlobalObjects::mpvplayer->getMediaInfo();
    QString displayText;
    for(auto iter=info.begin();iter!=info.end();++iter)
    {
        displayText+=QString("<font size=\"4\" face=\"Microsoft Yahei\" color=\"#18C0F1\">%0</font><br /><font size=\"3\" face=\"Microsoft Yahei\">%1</font>").arg(iter.key()).arg(iter.value().contains("General")?iter.value()["General"]+"<br />":"");
        QMap<QString,QString> &subInfo=iter.value();
        for(auto iter=subInfo.begin();iter!=subInfo.end();++iter)
        {
            if(iter.key()!="General")
            {
                displayText+=QString("<font size=\"3\" face=\"Microsoft Yahei\" color=\"#ECCC1E\">%0</font> :<font size=\"3\" face=\"Microsoft Yahei\"> %1</font><br />").arg(iter.key()).arg(iter.value());
            }
        }
    }
    infoText->setReadOnly(true);
    infoText->setText(displayText);
    QHBoxLayout *infoHLayout=new QHBoxLayout(this);
    infoHLayout->setContentsMargins(0,0,0,0);
    infoText->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::MinimumExpanding);
    infoHLayout->addWidget(infoText);
    resize(520*logicalDpiX()/96,400*logicalDpiY()/96);
}
