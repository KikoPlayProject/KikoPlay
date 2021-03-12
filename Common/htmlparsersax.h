#ifndef HTMLPARSERSAX_H
#define HTMLPARSERSAX_H
#include <QtCore>

class HTMLParserSax
{
    QByteArray cHtml;
    QByteArray::const_iterator begin,end;

    QByteArray nodeName;
    QMap<QByteArray,QByteArray> propertyMap;
    bool isStart;
    void parseNode();
    void parseComment();
    void skip();
public:
    HTMLParserSax(const QByteArray &content);
    void addData(const QByteArray &content);
    void seekTo(int pos);
    void readNext();
    inline bool atEnd(){return begin==end;}
    inline int curPos(){return begin-cHtml.cbegin();}
    const QByteArray readContentText();
    const QByteArray readContentUntil(const QString &node,bool isStart);

    inline bool isStartNode(){return isStart;}
    inline const QByteArray &currentNode(){return nodeName;}
    inline const QByteArray currentNodeProperty(const QByteArray &name){return propertyMap.value(name);}

};

#endif // HTMLPARSERSAX_H
