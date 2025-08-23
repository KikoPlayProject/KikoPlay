#include "animescrapingtask.h"
#include "Common/notifier.h"
#include "Common/logger.h"
#include "Extension/Script/scriptmanager.h"
#include "Extension/Script/libraryscript.h"
#include "MediaLibrary/animeworker.h"
#include "MediaLibrary/labelmodel.h"
#include "globalobjects.h"

AnimeScrapingTask::AnimeScrapingTask(Anime *anime, const MatchResult &match) : KTask{"anime_scraping"}, curAnime(anime), curMatch(match)
{

}

TaskStatus AnimeScrapingTask::runTask()
{
    auto script = GlobalObjects::scriptManager->getScript(curMatch.scriptId).staticCast<LibraryScript>();
    if (!script)
    {
        Logger::logger()->log(Logger::Script, QString("Match [%1] has Invalid ScriptId: %2").arg(curMatch.name, curMatch.scriptId));
        return TaskStatus::Failed;
    }
    AnimeLite ab;
    ab.name = curMatch.name;
    ab.scriptId = curMatch.scriptId;
    ab.scriptData = curMatch.scriptData;

    Anime *nAnime = new Anime;
    ScriptState state;
    int waitTimes = 0;
    const int maxWaitTimes = 10;
    setInfo(tr("Fetching Anime Info[%1]...").arg(curMatch.name), NM_HIDE);
    while (true)
    {
        if (_cancelFlag)
        {
            setInfo(tr("Task Canceled"), NM_ERROR | NM_HIDE);
            delete nAnime;
            return TaskStatus::Cancelled;
        }
        state = script->getDetail(ab, nAnime);
        if (state.isBusy())
        {
            if (waitTimes > maxWaitTimes)
            {
                delete nAnime;
                Logger::logger()->log(Logger::Script, QString("Match [%1] ScriptId: %2 wait timeout").arg(curMatch.name, curMatch.scriptId));
                return TaskStatus::Failed;
            }
            ++waitTimes;
            QThread::sleep(6);
        }
        else
        {
            break;
        }
    }
    if (!state)
    {
        delete nAnime;
        Logger::logger()->log(Logger::Script, QString("Match [%1:%2] ScriptId: %3 failed: %4").arg(curMatch.name, curMatch.ep.toString(), curMatch.scriptId, state.info));
        return TaskStatus::Failed;
    }

    QString animeName = AnimeWorker::instance()->addAnime(curAnime, nAnime);
    Anime *tAnime = AnimeWorker::instance()->getAnime(animeName);
    if (!tAnime)
    {
        setInfo(tr("Fetching Tags[%1] Failed").arg(animeName), NM_ERROR | NM_HIDE);
        return TaskStatus::Finished;
    }

    setInfo(tr("Fetching Tags[%1]...").arg(animeName), NM_HIDE);
    QStringList tags;
    waitTimes = 0;
    while (true)
    {
        if (_cancelFlag)
        {
            setInfo(tr("Fetch Tag Canceled"), NM_ERROR | NM_HIDE);
            return TaskStatus::Finished;
        }
        state = script->getTags(tAnime, tags);
        if (state.isBusy())
        {
            if (waitTimes > maxWaitTimes)
            {
                Logger::logger()->log(Logger::Script, QString("Match [%1] ScriptId: %2 fetch tag wait timeout").arg(curMatch.name, curMatch.scriptId));
                return TaskStatus::Finished;
            }
            ++waitTimes;
            QThread::sleep(6);
        }
        else
        {
            break;
        }
    }
    if (state && !tags.empty())
    {
        setInfo(tr("Fetch %1 Tags[%2]...").arg(tags.size()).arg(animeName), NM_HIDE);
        const QString animeName = tAnime->name();
        QMetaObject::invokeMethod(LabelModel::instance(), [=](){
            LabelModel::instance()->addCustomTags(animeName, tags);
        });
    }
    return TaskStatus::Finished;
}
