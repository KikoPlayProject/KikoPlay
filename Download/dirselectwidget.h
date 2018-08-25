#ifndef DIRSELECTWIDGET_H
#define DIRSELECTWIDGET_H

#include <QWidget>
class QComboBox;
class DirSelectWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DirSelectWidget(QWidget *parent = nullptr);
    ~DirSelectWidget();
    void setDir(const QString &dir);
    QString getDir() const;
    bool isValid();
    qint64 getFreeSpace(){return freeSpace;}
private:
    QComboBox *dirEdit;
    QStringList dirList;
    qint64 freeSpace;
    qint64 getAvailableBytes(const QString &dir);
};

#endif // DIRSELECTWIDGET_H
