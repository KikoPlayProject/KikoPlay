#ifndef SETTINGPAGE_H
#define SETTINGPAGE_H
#include <QWidget>
#include <QVariant>
#include <QHash>

class SettingItemArea : public QWidget
{
    Q_OBJECT
public:
    explicit SettingItemArea(const QString &title, QWidget *parent = nullptr);
    QSize sizeHint() const override;
    void addItem(const QString &name, QWidget *item, const QString &nameTip = "");
    void addItem(const QString &name, const QVector<QWidget *> &items, const QString &nameTip = "");
    void addItem(QWidget *item, Qt::Alignment align = Qt::Alignment());
private:
    QWidget *container{nullptr};
};

class SettingPage : public QWidget
{
    Q_OBJECT
public:
    using QWidget::QWidget;
    virtual void onAccept() {};
    virtual void onClose() {};
    const QHash<QString, QVariant> &getChangedValues() const {return changedValues;}
signals:
    void showBusyState(bool);
    void showMessage(const QString &msg, int type=1);
protected:
    QHash<QString, QVariant> changedValues;
};
#endif // SETTINGPAGE_H
