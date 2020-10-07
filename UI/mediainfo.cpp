#include "mediainfo.h"
#include <QTextEdit>
#include <QHBoxLayout>
#include "globalobjects.h"
#include "Play/Video/mpvplayer.h"
namespace  {
    QHash<QString, QString> kikoCommand({
        {"normalfont", GlobalObjects::normalFont}
    });
}
MediaInfo::MediaInfo(QWidget *parent) : CFramelessDialog(tr("Media Info"),parent)
{
    QTextEdit *infoText=new QTextEdit(this);
    /*
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
    */
    infoText->setReadOnly(true);
    //infoText->setText(displayText);
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
    }else {
        commandResult = QString("${%1}").arg(command);
    }
    if(commandStack.empty())
        textStack.back().text.append(commandResult);
    else
        commandStack.back().append(commandResult);
}
