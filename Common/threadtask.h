#ifndef THREADTASK_H
#define THREADTASK_H
#include <QtCore>
class ThreadTask
{
public:
    ThreadTask(QThread *thread) : taskThread(thread)
    {

    }
    template<typename Func>
    QVariant Run(Func task, bool blocking = false)
    {
        if (QThread::currentThread() == taskThread)
		{
			return task();
		}
		QObject obj;
		obj.moveToThread(taskThread);
        QVariant result;
        if (blocking)
        {
            QMetaObject::invokeMethod(&obj,[task, &result](){
                result = task();
            }, Qt::BlockingQueuedConnection);
        }
        else
        {
            QEventLoop eventLoop;
            QMetaObject::invokeMethod(&obj,[task, &result, &eventLoop](){
                result = task();
                QMetaObject::invokeMethod(&eventLoop, "quit", Qt::QueuedConnection);
            }, Qt::QueuedConnection);
            eventLoop.exec();
        }
        return result;
    }
    template<typename Func>
    void RunOnce(Func task)
    {
        if (QThread::currentThread() == taskThread)
        {
            task();
            return;
        }
        QObject *obj=new QObject();
        obj->moveToThread(taskThread);
        QMetaObject::invokeMethod(obj,[task,obj](){
           task();
           obj->deleteLater();
        });
    }
private:
    QThread *taskThread;
};
#endif // THREADTASK_H
