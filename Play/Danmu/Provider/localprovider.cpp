#include "localprovider.h"


void LocalProvider::LoadXmlDanmuFile(QString filePath, QList<DanmuComment *> &list)
{
    QFile xmlFile(filePath);
    bool ret=xmlFile.open(QIODevice::ReadOnly|QIODevice::Text);
    if(!ret)return;
    QXmlStreamReader reader(&xmlFile);
    while(!reader.atEnd())
    {
        if(reader.isStartElement() && reader.name()=="d")
        {
            QXmlStreamAttributes attributes=reader.attributes();
            if(attributes.hasAttribute("p"))
            {
                QStringList attrList=attributes.value("p").toString().split(',');
                if(attrList.length()>4)
                {
                    DanmuComment *danmu=new DanmuComment();
                    danmu->text=reader.readElementText();
                    danmu->time = attrList[0].toFloat() * 1000;
                    danmu->originTime=danmu->time;
                    int mode = attrList[1].toInt();
                    DanmuComment::DanmuType type = DanmuComment::Rolling;
                    if(mode==4) type = DanmuComment::Bottom;
                    else if(mode==5) type = DanmuComment::Top;
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
        reader.readNext();
    }
    xmlFile.close();
}
