#include "notifier.h"
Notifier::Notifier(QObject *parent) : QObject(parent)
{

}

void Notifier::cancelCallBack(NotifyInterface *interface)
{
    Notifier *notifier = getNotifier();
    emit notifier->cancelTrigger(interface->getType());
}

void Notifier::addNotify(Notifier::NotifyType nType, NotifyInterface *notify)
{
    notifyHash[nType].append(notify);
    notify->setType(notify->getType() | nType);
    notify->setCancelCallBack(cancelCallBack);
}

void Notifier::showMessage(NotifyType nType, const QString &content, int flag)
{
    auto &notifyList = notifyHash[nType];
    for(auto n: notifyList)
    {
        QMetaObject::invokeMethod(this, [=](){
            n->showMessage(content, flag);
        });
    }
}

Notifier *Notifier::getNotifier()
{
    static Notifier notifier;
    return &notifier;
}
