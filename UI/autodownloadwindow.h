#ifndef AUTODOWNLOADWINDOW_H
#define AUTODOWNLOADWINDOW_H

#include <QWidget>
class QTreeView;
class AutoDownloadWindow : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QColor logGeneralColor READ getGeneralColor WRITE setGeneralColor)
    Q_PROPERTY(QColor logResFindedColor READ getResFindedColor WRITE setResFindedColor)
    Q_PROPERTY(QColor logErrorColor READ getErrorColor WRITE setErrorColor)
public:
    explicit AutoDownloadWindow(QWidget *parent = nullptr);

    QColor getGeneralColor() const;
    void setGeneralColor(const QColor& color);
    QColor getResFindedColor() const;
    void setResFindedColor(const QColor& color);
    QColor getErrorColor() const;
    void setErrorColor(const QColor& color);
private:
    QTreeView *ruleView, *logView, *urlView;
    void setupActions();
signals:
    void addTask(const QStringList &uris, bool directly, const QString &path);
public slots:
};

#endif // AUTODOWNLOADWINDOW_H
