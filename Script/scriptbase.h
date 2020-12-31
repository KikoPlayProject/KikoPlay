#ifndef SCRIPTBASE_H
#define SCRIPTBASE_H

#include <QObject>
#include <QHash>
class ScriptBase : public QObject
{
    Q_OBJECT
public:
    explicit ScriptBase(QObject *parent = nullptr);

public:
    struct ScriptSettingItem
    {
        QString title;
        QString description;
        enum class ValueType
        {
            SS_STRING, SS_STRINGLIST
        };
        ValueType vtype;

    };
protected:

    QHash<QString, QString> scriptMeta;



signals:

};

#endif // SCRIPTBASE_H
