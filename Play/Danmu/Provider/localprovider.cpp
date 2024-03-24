#include "localprovider.h"
#include "Common/htmlparsersax.h"

void LocalProvider::LoadXmlDanmuFile(QString filePath, QVector<DanmuComment *> &list)
{
    QFile xmlFile(filePath);
    bool ret = xmlFile.open(QIODevice::ReadOnly|QIODevice::Text);
    if (!ret) return;

    const QByteArray content = xmlFile.readAll();

    HTMLParserSax parser(content);
    while (!parser.atEnd())
    {
        if (parser.isStartNode() && parser.currentNode() == "d")
        {
            const QByteArray attr = parser.currentNodeProperty("p");
            if (!attr.isEmpty())
            {
                auto attrList = attr.split(',');
                if (attrList.length() > 4)
                {
                    const QByteArray text = parser.readContentText();
                    QByteArray danmuText;
                    for(char ch : text)
                    {
                        if ((ch>=0x0 && ch<=0x8) || (ch>=0xb && ch<=0xc) || (ch>=0xe && ch<=0x1f))
                            continue;
                        danmuText.append(ch);
                    }
                    if (danmuText.isEmpty())
                    {
                        parser.readNext();
                        continue;
                    }

                    DanmuComment *danmu=new DanmuComment();
                    danmu->text = danmuText;
                    danmu->time = attrList[0].toFloat() * 1000;
                    danmu->originTime=danmu->time;
                    int mode = attrList[1].toInt();
                    DanmuComment::DanmuType type = DanmuComment::Rolling;
                    if (mode==4) type = DanmuComment::Bottom;
                    else if (mode==5) type = DanmuComment::Top;
                    danmu->type =type;
                    danmu->color=attrList[3].toInt();
                    if(attrList.length()>4)
                        danmu->date=attrList[4].toLongLong();
                    if(attrList.length()>6)
                        danmu->sender=attrList[6];
                    switch (attrList[2].toInt())
                    {
                    case 25:
                        danmu->fontSizeLevel=DanmuComment::Normal;
                        break;
                    case 18:
                        danmu->fontSizeLevel=DanmuComment::Small;
                        break;
                    case 36:
                        danmu->fontSizeLevel=DanmuComment::Large;
                        break;
                    default:
                        danmu->fontSizeLevel=DanmuComment::Normal;
                        break;
                    }
                    list.append(danmu);
                }
            }
        }
        parser.readNext();
    }
    xmlFile.close();
}
