#ifndef AUTODOWNLOADWINDOW_H
#define AUTODOWNLOADWINDOW_H

#include <QWidget>
class QTreeView;
class AutoDownloadWindow : public QWidget
{
    Q_OBJECT
public:
    explicit AutoDownloadWindow(QWidget *parent = nullptr);

private:
    QTreeView *ruleView, *logView, *urlView;
    void setupActions();
signals:
    void addTask(const QStringList &uris, bool directly, const QString &path);
public slots:
};

#endif // AUTODOWNLOADWINDOW_H
