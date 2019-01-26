#include "common.h"
#include "globalobjects.h"
#include "Render/danmurender.h"
#define MAX_POOL_COUNT 100
DanmuObject *DanmuObject::head=nullptr;
int DanmuObject::poolCount=0;

bool BlockRule::blockTest(DanmuComment *comment)
{
    if(!enable)return false;
    QString *testStr;
    bool testResult(false);
    switch (blockField)
    {
    case DanmuText:
        testStr=&comment->text;
        break;
    case DanmuSender:
        testStr=&comment->sender;
        break;
    case DanmuColor:
        testStr=new QString(QString::number(comment->color,16));
        break;
    }
    switch (relation)
    {
    case Equal:
    case NotEqual:
    {
        if(isRegExp)
        {
            if(re.isNull())re.reset(new QRegExp(content));
            if(re->indexIn(*testStr)!=-1)
            {
                testResult=re->matchedLength()==testStr->length()?true:false;
            }
        }
        else
        {
            testResult=(*testStr==content);
        }
        if(relation==Relation::NotEqual)testResult=!testResult;
    }
        break;
    case Contain:
    {
        if(isRegExp)
        {
            if(re.isNull())re.reset(new QRegExp(content));
            testResult=(re->indexIn(*testStr)!=-1)?true:false;
        }
        else
        {
            testResult=testStr->contains(content);
        }
    }
        break;
    default:
        break;
    }
    if(blockField==DanmuColor)delete testStr;
    return testResult;
}

DanmuObject::~DanmuObject()
{
    GlobalObjects::danmuRender->refDesc(drawInfo);
}

void *DanmuObject::operator new(size_t sz)
{
    if(head)
    {
        void *tmp=(void *)head;
        head=head->next;
        poolCount--;
        return tmp;
    }
    else
    {
        void *tmp=malloc(sz);
        if(!tmp) throw std::bad_alloc();
        return tmp;
    }
}

void DanmuObject::operator delete(void *p)
{
    if(poolCount>MAX_POOL_COUNT)
    {
        free(p);
    }
    else
    {
        DanmuObject *obj=static_cast<DanmuObject *>(p);
        obj->next=head;
        head=obj;
        poolCount++;
    }
}

void DanmuObject::DeleteObjPool()
{
    DanmuObject *p;
    for(p=head;p!=nullptr;p=head)
    {
        head=head->next;
        free((void *)p);
    }
    poolCount=0;
}
