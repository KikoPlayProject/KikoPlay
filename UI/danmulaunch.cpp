#include "danmulaunch.h"
#include "widgets/colorpicker.h"
#include <QComboBox>
#include <QLineEdit>
#include <QListWidget>
#include <QGridLayout>
#include <QLabel>
#include "globalobjects.h"
#include "Play/Video/mpvplayer.h"
#include "Play/Danmu/danmuprovider.h"
#include "Play/Danmu/Manager/danmumanager.h"
#include "Play/Danmu/Manager/pool.h"
#include "Script/scriptmanager.h"
#include "Script/scriptlogger.h"
#include "Common/notifier.h"
#define ScriptIdRole Qt::UserRole+1

DanmuLaunch::DanmuLaunch(QWidget *parent) : CFramelessDialog(tr("Launch"), parent, true, true, false), curTime(0)
{
    QList<QColor> colors
    {
      Qt::white, Qt::yellow, Qt::red, Qt::blue, Qt::green,
      Qt::black, Qt::cyan, Qt::magenta, Qt::gray, Qt::darkYellow
    };
    QLabel *typeTip = new QLabel(tr("Danmu Type"), this);
    QLabel *sizeTip = new QLabel(tr("Danmu Size"), this);
    colorPicker = new ColorPicker(colors, this);
    typeCombo = new QComboBox(this);
    fontSizeCombo = new QComboBox(this);
    textEdit = new QLineEdit(this);
    scriptList = new QListWidget(this);
    QGridLayout *launchGLayout = new QGridLayout(this);
    launchGLayout->addWidget(colorPicker, 0, 0);
    launchGLayout->addWidget(typeTip, 0, 1);
    launchGLayout->addWidget(typeCombo, 0, 2);
    launchGLayout->addWidget(sizeTip, 0, 3);
    launchGLayout->addWidget(fontSizeCombo, 0, 4);
    launchGLayout->addWidget(textEdit, 1, 0, 1, 6);
    launchGLayout->addWidget(scriptList, 2, 0, 1, 6);
    launchGLayout->setRowStretch(2, 1);
    launchGLayout->setColumnStretch(5, 1);
    launchGLayout->setContentsMargins(0, 0, 0, 0);

    colorPicker->setColor(Qt::white);
    typeCombo->addItems({tr("Roll"), tr("Top"), tr("Bottom")});
    fontSizeCombo->addItems({tr("Normal"), tr("Small"), tr("Large")});
    setTime();

    QObject::connect(textEdit, &QLineEdit::returnPressed, this, &DanmuLaunch::onAccept);

    QObject::connect(GlobalObjects::danmuProvider, &DanmuProvider::sourceCheckDown, this, [=](const QString &poolId,  const QStringList &supportedScripts){
        scriptList->clear();
        curPoolId = poolId;
        for(const QString &sid : supportedScripts)
        {
            auto script = GlobalObjects::scriptManager->getScript(sid);
            if(!script) continue;
            QListWidgetItem *item = new QListWidgetItem(scriptList);
            item->setText(script->name());
            item->setData(ScriptIdRole, sid);
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        }
        if(scriptList->count()>0)
        {
            scriptList->item(0)->setCheckState(Qt::CheckState::Checked);
        }
    });
    QObject::connect(GlobalObjects::danmuProvider, &DanmuProvider::danmuLaunchStatus,
                     [=](const QString &poolId, const QStringList &scriptIds, const QStringList &status, DanmuComment *comment){
        QScopedPointer<DanmuComment> c(comment);
        Pool *pool = GlobalObjects::danmuManager->getPool(poolId, false);
        if(!pool) return;
        QStringList results;
        for(int i = 0; i<status.size(); ++i)
        {
            results.append(QString("[%1]: %2").arg(scriptIds[i], status[i].isEmpty()?tr("Success"):tr("Faild, %1").arg(status[i])));
        }
        QString poolInfo(QString("%1 %2").arg(pool->animeTitle(), pool->toEp().toString()));
        int cmin=c->originTime/1000/60;
        int cls=c->originTime/1000-cmin*60;
        QString commentInfo(QString("[%1:%2]%3").arg(cmin,2,10,QChar('0')).arg(cls,2,10,QChar('0')).arg(c->text));
        QString msg(QString("%1\n%2\n%3").arg(poolInfo, commentInfo, results.join('\n')));
        Notifier::getNotifier()->showMessage(Notifier::PLAYER_NOTIFY, msg);
        ScriptLogger::instance()->appendText(msg);
    });
}

void DanmuLaunch::show()
{
    setTime();
    CFramelessDialog::show();
}

void DanmuLaunch::onAccept()
{
    Pool *pool = GlobalObjects::danmuManager->getPool(curPoolId, false);
    if(!pool)
    {
        showMessage(tr("Invalid Danmu Pool"), NM_ERROR | NM_HIDE);
        return;
    }
    QList<DanmuSource> srcs;
    for(auto src : pool->sources())
    {
        srcs.append(src);
    }
    if(srcs.size()==0)
    {
        showMessage(tr("Danmu Pool has no Sources"), NM_ERROR | NM_HIDE);
        return;
    }
    int scriptCount = scriptList->count();
    QStringList scriptIds;
    for(int i = 0; i < scriptCount; ++i)
    {
        QListWidgetItem *item = scriptList->item(i);
        if(item->checkState() == Qt::CheckState::Checked)
        {
            scriptIds.append(item->data(ScriptIdRole).toString());
        }
    }
    if(scriptIds.size() == 0)
    {
        showMessage(tr("No Script Selected"), NM_ERROR | NM_HIDE);
        return;
    }
    QString text(textEdit->text().trimmed());
    if(text.isEmpty())
    {
        showMessage(tr("Danmu Text Should not be empty"), NM_ERROR | NM_HIDE);
        return;
    }
    const int types[] = {1, 5, 4};
    int type = types[typeCombo->currentIndex()];
    int fontSize = fontSizeCombo->currentIndex();
    QColor color = colorPicker->color();
    DanmuComment *comment = new DanmuComment;
    comment->originTime = comment->time = curTime * 1000;  //ms
    comment->color = (int)(color.red()<<16) | (color.green()<<8) | color.blue();
    comment->text = text;
    comment->setType(type);
    comment->fontSizeLevel = DanmuComment::FontSizeLevel(fontSize);
    comment->date = QDateTime::currentDateTime().toSecsSinceEpoch();
    GlobalObjects::danmuProvider->launch(scriptIds, curPoolId, srcs, comment);
    CFramelessDialog::onAccept();
}

void DanmuLaunch::setTime()
{
    curTime = GlobalObjects::mpvplayer->getTime();
    int cmin=curTime/60;
    int cls=curTime-cmin*60;
    CFramelessDialog::setTitle(tr("Launch - %1:%2").arg(cmin,2,10,QChar('0')).arg(cls,2,10,QChar('0')));
}

int DanmuLaunch::exec()
{
    bool restorePlayState = false;
    if (GlobalObjects::mpvplayer->getState() == MPVPlayer::Play)
    {
        restorePlayState = true;
        GlobalObjects::mpvplayer->setState(MPVPlayer::Pause);
    }
    setTime();
    int ret = CFramelessDialog::exec();
    if(restorePlayState)
    {
        GlobalObjects::mpvplayer->setState(MPVPlayer::Play);
    }
    return ret;
}
