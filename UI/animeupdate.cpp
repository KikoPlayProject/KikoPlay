#include "animeupdate.h"
#include <QMessageBox>
#include "MediaLibrary/animeinfo.h"
#include "globalobjects.h"
#include "MediaLibrary/animeworker.h"
#include "MediaLibrary/animeprovider.h"
AnimeUpdate::AnimeUpdate(Anime *anime, QWidget *parent)
    : CFramelessDialog(tr("Anime Update"),parent,false,false,false)
{
    QTimer::singleShot(0,[this,anime](){
        showBusyState(true);
        Anime *nAnime = new Anime;
        ScriptState state = GlobalObjects::animeProvider->getDetail(anime->toLite(), nAnime);
        showBusyState(false);
        if(!state)
        {
            QMessageBox::information(this,tr("Error"), state.info);
            delete nAnime;
        } else {
            AnimeWorker::instance()->addAnime(anime, nAnime);
        }
        CFramelessDialog::onAccept();
    });
}
