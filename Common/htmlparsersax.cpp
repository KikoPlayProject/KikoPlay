#include "htmlparsersax.h"

void HTMLParserSax::parseNode()
{
    begin++;
    if(begin==end)return;
    if(*begin=='!')
    {
        parseComment();
		nodeName.clear();
		propertyMap.clear();
        return;
    }
    QByteArray::const_iterator iter=begin;
    nodeName.clear();
    isStart=!(*iter=='/');
    if(!isStart)iter++;
    while (iter!=end && QChar(*iter).isLetterOrNumber()) nodeName+=*iter++;
    propertyMap.clear();
    while(iter!=end && *iter!='>')
    {
        while(iter!=end && QChar(*iter).isSpace())iter++;
        if(*iter=='/')
        {
            while(iter!=end && *iter!='>')iter++;
            break;
        }
        else if(*iter!='>')
        {
            QByteArray propertyName,propertyVal;
            while (iter!=end && *iter!='=' && *iter!='>') propertyName+=*iter++;
            if(iter!=end && *iter!='>')
            {
                iter++;
                QChar q(*iter++);
                while (iter!=end && *iter!=q) propertyVal+=*iter++;
                propertyMap.insert(propertyName,propertyVal);
                if(iter!=end) iter++;
            }
        }
    }
    if(iter!=end && *iter=='>')iter++;
    begin=iter;
}

void HTMLParserSax::parseComment()
{
    int commentState=0;
    while(begin!=end)
    {
        if(*begin=='-')
        {
            if(commentState==0)commentState=1;
            else if(commentState==1)commentState=2;
        }
        else if(*begin=='>' && commentState==2)
        {
            begin++;
            break;
        }
        else
        {
            commentState=0;
        }
        begin++;
    }
}

void HTMLParserSax::skip()
{
    while(begin!=end)
    {
        if(*begin!='<')begin++;
        else return;
    }
}

HTMLParserSax::HTMLParserSax(const QByteArray &content):cHtml(content), isStart(false)
{
    begin=content.cbegin();
    end=content.cend();
}

void HTMLParserSax::seekTo(int pos)
{
    pos = qBound<int>(0, pos, cHtml.size());
    begin=cHtml.begin()+pos;
    nodeName="";
    isStart=false;
    propertyMap.clear();
}

void HTMLParserSax::readNext()
{
    if(begin==end)return;
    if(QChar(*begin).isSpace())
        while(begin!=end && QChar(*begin).isSpace())begin++;
    if(*begin=='<')
    {
        parseNode();
    }
    else
    {
        skip();
        isStart=false;
        nodeName.clear();
        propertyMap.clear();
    }
}

const QByteArray HTMLParserSax::readContentText()
{
    int contentState=0;
    QByteArray text;
    while(begin!=end)
    {
        if(*begin=='<')
        {
            if(contentState==0)contentState=1;
        }
        else if(*begin=='/' && contentState==1)
        {
            begin-=1;
            break;
        }
        else
        {
            if(contentState==1)
                text+='<';
            text+=*begin;
        }
        begin++;
    }
    return text;
}

const QByteArray HTMLParserSax::readContentUntil(const QString &node, bool isStart)
{
    int startPos = curPos();
    while(begin!=end)
    {
        readNext();
        if(nodeName==node && this->isStart==isStart) break;
    }
    return cHtml.mid(startPos,curPos()-startPos);
}
