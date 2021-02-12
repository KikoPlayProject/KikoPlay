#ifndef HTMLPARSERSAX_H
#define HTMLPARSERSAX_H
#include <QtCore>

class HTMLParserSax
{
    const QString cHtml;
    QString::const_iterator begin,end;

    QString nodeName;
    QMap<QString,QString> propertyMap;
    bool isStart;
    void parseNode();
    void parseComment();
    void skip();
public:
    HTMLParserSax(const QString &content);
    void seekTo(int pos);
    void readNext();
    inline bool atEnd(){return begin==end;}
    inline int curPos(){return begin-cHtml.begin();}
    const QString readContentText();
    const QString readContentUntil(const QString &node,bool isStart);

    inline bool isStartNode(){return isStart;}
    inline const QString &currentNode(){return nodeName;}
    inline const QString currentNodeProperty(const QString &name){return propertyMap.value(name);}

};

#endif // HTMLPARSERSAX_H
