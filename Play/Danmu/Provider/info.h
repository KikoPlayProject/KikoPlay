#ifndef INFO_H
#define INFO_H
#include <QtCore>
struct DanmuSourceItem
{
    QString title;
    QString description;
    QString strId;
    int danmuCount;
    int id;
    int subId;
    int extra;
	int delay;
};
Q_DECLARE_OPAQUE_POINTER(DanmuSourceItem *)
struct DanmuAccessResult
{
    bool error;
    QString errorInfo;
	QString providerId;
    QList<DanmuSourceItem> list;
};
#endif // INFO_H
