#ifndef PLAYCONTEXT_H
#define PLAYCONTEXT_H

#include <QObject>
#include <QImage>
struct PlayListItem;

class PlayContext : public QObject
{
    Q_OBJECT
public:
    PlayContext &operator=(const PlayContext&) = delete;
    PlayContext(const PlayContext&) = delete;
    static PlayContext *context();
public:
    void playItem(const PlayListItem *item);
    void playURL(const QString &url);
    void clear();
public:
    QString path;
    int duration;
    int playtime;
    bool seekable;
    QImage stopFrame;
    const PlayListItem *curItem;
private:
    explicit PlayContext(QObject *parent = nullptr);
signals:

};

#endif // PLAYCONTEXT_H
