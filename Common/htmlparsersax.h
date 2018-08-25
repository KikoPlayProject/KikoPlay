#ifndef HTMLPARSERSAX_H
#define HTMLPARSERSAX_H
#include <QtCore>

class HTMLParserSax
{
    QString::const_iterator begin,end;

    QString nodeName;
    QMap<QString,QString> propertyMap;
    bool isStart;
    void parseNode();
    void parseComment();
    void skip();
public:
    HTMLParserSax(const QString &content);
    void readNext();
    inline bool atEnd(){return begin==end;}
    QString readContentText();

    inline bool isStartNode(){return isStart;}
    inline const QString &currentNode(){return nodeName;}
    inline QString currentNodeProperty(const QString &name){return propertyMap.value(name);}

};

#endif // HTMLPARSERSAX_H
