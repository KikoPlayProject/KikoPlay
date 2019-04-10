#ifndef THREADTASK_H
#define THREADTASK_H
#include <QtCore>
class ThreadTask : public QObject
{
    Q_OBJECT
public:
    ThreadTask(QThread *thread, QObject *parent=nullptr):QObject(parent),taskId(0),taskThread(thread)
    {

    }
    template<typename Func>
    QVariant Run(Func task)
    {
        if (QThread::currentThread() == taskThread)
		{
			return task();
		}
		QObject obj;
		obj.moveToThread(taskThread);
		QEventLoop eventLoop;
		int tId = this->taskId;
		QVariant result;
        QObject::connect(this,&ThreadTask::TaskDone,&eventLoop,[&eventLoop,&result,tId](int taskId, QVariant ret){
            if(tId==taskId)
            {
                result=ret;
                eventLoop.quit();
            }
        });
        QMetaObject::invokeMethod(&obj,[task,tId,this](){
            emit this->TaskDone(tId,task());
        });
        tId++;
        eventLoop.exec();
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
signals:
    void TaskDone(int taskId, QVariant ret);
private:
    int taskId;
    QThread *taskThread;
};
#endif // THREADTASK_H
