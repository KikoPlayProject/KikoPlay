#include "bangumiupdate.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include "MediaLibrary/animeinfo.h"
#include "globalobjects.h"
#include "MediaLibrary/animelibrary.h"
BangumiUpdate::BangumiUpdate(Anime *anime, QWidget *parent)
    : CFramelessDialog(tr("Bangumi Update"),parent,false,false,false)
{
    QLabel *tipLabel=new QLabel(this);
    QHBoxLayout *buHLayout=new QHBoxLayout(this);
    buHLayout->addWidget(tipLabel);
    QObject::connect(GlobalObjects::library,&AnimeLibrary::downloadDetailMessage,this,[tipLabel](const QString &msg){
       tipLabel->setText(msg);
    });
    QTimer::singleShot(0,[this,anime](){
        showBusyState(true);
        QString errInfo;
        GlobalObjects::library->downloadDetailInfo(anime,anime->bangumiID,&errInfo);
        showBusyState(false);
        if(!errInfo.isEmpty())
        {
            QMessageBox::information(this,tr("Error"),errInfo);
        }
        CFramelessDialog::onAccept();
    });
}
