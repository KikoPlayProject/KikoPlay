#include "episodeeditor.h"
#include <QTreeView>
#include <QHeaderView>
#include <QPushButton>
#include <QGridLayout>
#include <QFileDialog>
#include <QMessageBox>
#include "MediaLibrary/episodesmodel.h"
#include "globalobjects.h"
#include "Play/Video/mpvplayer.h"
#include "Common/network.h"
EpisodeEditor::EpisodeEditor(Anime *anime, QWidget *parent) : CFramelessDialog(tr("Episodes"),parent)
{
    QTreeView *episodeView=new QTreeView(this);
    episodeView->setRootIsDecorated(false);
    episodeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    episodeView->setItemDelegate(new EpItemDelegate(&epNames,this));
    episodeView->setFont(font());
    epModel=new EpisodesModel(anime,this);
    episodeView->setModel(epModel);
    episodeView->setAlternatingRowColors(true);
    QPushButton *add=new QPushButton(tr("Add Episode(s)"),this);
    QObject::connect(add,&QPushButton::clicked,[this](){
        QStringList files = QFileDialog::getOpenFileNames(this,tr("Select media files"),"",
                                                         QString("Video Files(%1);;All Files(*) ").arg(GlobalObjects::mpvplayer->videoFileFormats.join(" ")));
        if (files.count())
        {
            QFileInfo fi;
            for(auto &file:files)
            {
                fi.setFile(file);
                epModel->addEpisode(fi.baseName(),file);
            }
        }
    });
    QPushButton *remove=new QPushButton(tr("Remove Episode(s)"),this);
    QObject::connect(remove,&QPushButton::clicked,[episodeView,this](){
        auto selection = episodeView->selectionModel()->selectedRows();
        if (selection.size() == 0)return;
        epModel->removeEpisodes(selection);
    });
    QPushButton *getEpNames=new QPushButton(tr("Get Epsoide Names"),this);
    QObject::connect(getEpNames,&QPushButton::clicked,[this,anime,getEpNames](){
        if(anime->bangumiID==-1)return;
        QString epUrl(QString("https://api.bgm.tv/subject/%1/ep").arg(anime->bangumiID));
        try
        {
            getEpNames->setText(tr("Getting..."));
            getEpNames->setEnabled(false);
            this->showBusyState(true);
            QString str(Network::httpGet(epUrl,QUrlQuery(),QStringList()<<"Accept"<<"application/json"));
            QJsonDocument document(Network::toJson(str));
            QJsonObject obj = document.object();
            QJsonArray epArray=obj.value("eps").toArray();
            for(auto iter=epArray.begin();iter!=epArray.end();++iter)
            {
                QJsonObject epobj=(*iter).toObject();
                epNames.append(QString("No.%0 %1(%2)").arg(epobj.value("sort").toInt()).arg(epobj.value("name").toString()).arg(epobj.value("name_cn").toString()));
            }
            getEpNames->setText(tr("Getting Done"));

        }
        catch(Network::NetworkError &error)
        {
            QMessageBox::information(this,tr("Error"),error.errorInfo);
            getEpNames->setEnabled(true);
            getEpNames->setText(tr("Get Epsoide Names"));
        }
        this->showBusyState(false);
    });
    QGridLayout *epGLayout=new QGridLayout(this);
    epGLayout->addWidget(add,0,0);
    epGLayout->addWidget(remove,0,1);
    epGLayout->addWidget(getEpNames,0,2);
    epGLayout->addWidget(episodeView,1,0,1,4);
    epGLayout->setRowStretch(1,1);
    epGLayout->setColumnStretch(3,1);
    epGLayout->setContentsMargins(0, 0, 0, 0);
    resize(400*logicalDpiX()/96, 320*logicalDpiY()/96);
    QHeaderView *epHeader = episodeView->header();
    epHeader->setFont(font());
    epHeader->resizeSection(0, 160*logicalDpiX()/96);
    epHeader->resizeSection(1, 100*logicalDpiX()/96);
}

bool EpisodeEditor::episodeChanged()
{
    return epModel->episodeChanged;
}
