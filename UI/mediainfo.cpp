#include "mediainfo.h"
#include <QTextEdit>
#include <QHBoxLayout>
#include "globalobjects.h"
#include "Play/Video/mpvplayer.h"
MediaInfo::MediaInfo(QWidget *parent) : CFramelessDialog(tr("Media Info"),parent)
{
    QTextEdit *infoText=new QTextEdit(this);
    infoText->setReadOnly(true);
    infoText->setText(expandMediaInfo());
    QHBoxLayout *infoHLayout=new QHBoxLayout(this);
    infoHLayout->setContentsMargins(0,0,0,0);
    infoText->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::MinimumExpanding);
    infoHLayout->addWidget(infoText);
    resize(520*logicalDpiX()/96,400*logicalDpiY()/96);
}

QString MediaInfo::expandMediaInfo()
{
    static QString mediaInfoTemplate;
    if(mediaInfoTemplate.isEmpty())
    {
        QFile infoTempFile(":/res/mediainfo");
        infoTempFile.open(QFile::ReadOnly);
        if(infoTempFile.isOpen())
        {
            mediaInfoTemplate = infoTempFile.readAll();
        }
    }
    int state = 0;
    QList<TextBlock> textStack;
    QList<QString> commandStack;
    textStack.push_back(TextBlock());

    for(QChar c : mediaInfoTemplate)
    {
        if(state == 0)
        {
            if(c=='$')
            {
                state = 1;
            }
            else textStack.back().text.append(c);
        }
        else if(state == 1)
        {
            if(c=='{')
            {
                state=2;
                commandStack.push_back("");
            }
            else if(!commandStack.isEmpty())
            {
                state = 2;
                commandStack.back().append(c);
            }
            else
            {
                state=0;
                textStack.back().text.append("$");
                textStack.back().text.append(c);
            }
        }
        else if(state == 2)
        {
            if(c=='}')
            {
                evalCommand(commandStack, textStack);
                if(commandStack.isEmpty())
                    state=0;
            }
            else if(c=='$')
            {
                state = 1;
            }
            else
            {
                commandStack.back().append(c);
            }

        }
    }
    return GlobalObjects::mpvplayer->expandMediaInfo(textStack.back().text);
}

void MediaInfo::evalCommand(QList<QString> &commandStack, QList<TextBlock> &textStack)
{
    static QHash<QString, QString> kikoCommand({
        {"normalfont", GlobalObjects::normalFont},
        {"tr:title", tr("Title")},
        {"tr:file-size", tr("File Size")},
        {"tr:date-created", tr("Date Created")},
        {"tr:file", tr("File")},
        {"tr:file-format", tr("File Format")},
        {"tr:duration", tr("Duration")},
        {"tr:video-output", tr("Video Output")},
        {"tr:demuxer", tr("Demuxer")},
        {"tr:bitrate", tr("Bitrate")},
        {"tr:video", tr("Video")},
        {"tr:audio-output", tr("Audio Output")},
        {"tr:sample-rate", tr("Sample Rate")},
        {"tr:channels", tr("Channels")},
        {"tr:audio", tr("Audio")},
        {"tr:meta-data", tr("Meta Data")},
        {"tr:video-format", tr("Video Format")},
        {"tr:video-size", tr("Video Size")},
        {"tr:display-size", tr("Display Size")},
        {"tr:audio-format", tr("Audio Format")},
        {"tr:channel-count", tr("Channel Count")},
        {"tr:chapters", tr("Chapters")},
        {"tr:tracks", tr("Tracks")},
        {"tr:language", tr("Language")},
        {"tr:selected", tr("Selected")}
    });
    QFileInfo fi(GlobalObjects::mpvplayer->getCurrentFile());
    kikoCommand["date-created"] = fi.birthTime().toString();
    kikoCommand["audio-trackcount"] = QString::number(GlobalObjects::mpvplayer->getTrackList(MPVPlayer::AudioTrack).size());
    kikoCommand["dwidth"] = QString::number(GlobalObjects::mpvplayer->width());
    kikoCommand["dheight"] = QString::number(GlobalObjects::mpvplayer->height());

    QString command = commandStack.back();
    commandStack.pop_back();
    QString commandResult;
    if(command.startsWith("kiko:") && kikoCommand.contains(command.mid(5))) {
        commandResult = kikoCommand[command.mid(5)];
    } else if(command.startsWith("mpv:")) {
        bool hasError;
        QString mpvProperty(GlobalObjects::mpvplayer->getMPVProperty(command.mid(4), hasError));
        if(!hasError) commandResult = mpvProperty;
    } else if(command.startsWith("loop:")) {
        QString loop = command.mid(5);
        int varPos = loop.indexOf('=');
        do
        {
            if(varPos <= 0) break;
            QString loopVar = loop.left(varPos).trimmed();
            if(loopVar.isEmpty()) break;
            QStringList loopConds = loop.mid(varPos+1).split(',');
            if(loopConds.size()<2) break;
            bool ok;
            int start = loopConds[0].toInt(&ok);
            if(!ok) break;
            int end = loopConds[1].toInt(&ok);
            if(!ok) break;
            int step = 1;
            if(loopConds.size()>2)
            {
                step = loopConds[2].toInt(&ok);
                if(!ok) break;
            }
            TextBlock block;
            block.blockVar = loopVar;
            block.start = start; block.end = end; block.step = step;
            textStack.push_back(block);
        }while(false);
        return;
    } else if(command.startsWith("endloop")) {
        if(textStack.size()>1)
        {
            auto &block = textStack.back();
            QString content;
            QString var = QString("${var:%1}").arg(block.blockVar);
            for(int i=block.start; block.start<=block.end?(i<block.end):(i>=block.end);i+=block.step)
            {
                content.append(QString(block.text).replace(var, QString::number(i)));
            }
            textStack.pop_back();
            textStack.back().text.append(content);
        }
        return;
    } else if(command.startsWith("if:")) {
        QString ifcond = command.mid(3);
        int lhsPos = ifcond.indexOf(QRegExp("([<>=])|(>=)|(<=)"));
        do
        {
            if(lhsPos <= 0) break;
            QChar op = ifcond[lhsPos];
            QString lhs = ifcond.left(lhsPos).trimmed();
            if(lhs.isEmpty()) break;
            int rhsPos = lhsPos + 1;
            if(rhsPos >= ifcond.length()) break;
            bool hasEq = false;
            if(rhsPos + 1<ifcond.length() && ifcond[rhsPos]=='=')
            {
                rhsPos += 1;
                hasEq = true;
            }
            QString rhs = ifcond.mid(rhsPos).trimmed();
            if(rhs.isEmpty()) break;
            bool lNum = true, rNum = true;
            double ilhs = lhs.toDouble(&lNum);
            double irhs = rhs.toDouble(&rNum);
            TextBlock block;
            if(lNum && rNum)
            {
                block.cond = cp(ilhs, irhs, op.toLatin1(), hasEq);
            }
            else
            {
                block.cond = cp(lhs, rhs, op.toLatin1(), hasEq);
            }
            textStack.push_back(block);
        }while(false);
        return;
    } else if(command.startsWith("endif")) {
           if(textStack.size()>1)
           {
               QString content = textStack.back().text;
               bool cond = textStack.back().cond;
               textStack.pop_back();
               if(cond)
               {
                   textStack.back().text.append(content);
               }
           }
           return;
    } else {
        commandResult = QString("${%1}").arg(command);
    }
    if(commandStack.empty())
        textStack.back().text.append(commandResult);
    else
        commandStack.back().append(commandResult);
}
