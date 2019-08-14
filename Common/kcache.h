#ifndef KCACHE_H
#define KCACHE_H

#include <QHash>
#include <QDataStream>
class KCache
{
public:
    explicit KCache(int mSize = 128):maxSize(mSize), h(nullptr), t(nullptr){}

    template<typename T>
    void put(const QString &key, const T &value)
    {
        QByteArray buff;
        QDataStream stream(&buff, QIODevice::WriteOnly);
        stream<<value;
        Node *node;
        auto iter = hash.find(key);
        if(iter!=hash.end())
        {
            node=&iter.value();
            node->value=buff;
            take(node);
        }
        else
        {
            Node n;
            n.value=buff;
            auto nIter = hash.insert(key, n);
            node = &nIter.value();
            node->key=&nIter.key();
        }
        prepend(node);
        if(hash.count()>maxSize) remove();
    }
    template<typename T>
    T *get(const QString &key)
    {
        auto iter = hash.find(key);
        if(iter==hash.end()) return nullptr;
        Node *node = &iter.value();
        T *obj=new T;
        QDataStream stream(&node->value, QIODevice::ReadOnly);
        stream>>*obj;
        if(node==h) return obj;
        take(node);
        prepend(node);
        return obj;
    }

private:
    int maxSize;
    struct Node
    {
        Node():p(nullptr),n(nullptr),key(nullptr){}
        Node *p, *n;
        const QString *key;
        QByteArray value;
    };
    QHash<QString, Node> hash;
    Node *h, *t;
    inline void take(Node *node)
    {
        if(node==h) h=node->n;
        if(node==t) t=node->p;
        if(node->n) node->n->p=node->p;
        if(node->p) node->p->n=node->n;
        node->n=nullptr;
        node->p=nullptr;
    }
    inline void prepend(Node *node)
    {
        node->n=h;
        if(h) h->p=node;
        h=node;
        if(!t) t=h;
    }
    void remove()
    {
        int i=maxSize/2;
        Node *cur=t;
        while(i>0 && cur)
        {
            Node *n=cur->p;
            if(n) n->n=nullptr;
            hash.remove(*cur->key);
            cur=n;
            --i;
        }
        t=cur;
    }
};

#endif // KCACHE_H
