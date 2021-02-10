#ifndef LRUCACHE_H
#define LRUCACHE_H
#include <QHash>
#include <QMutex>
template <typename K, typename V>
class LRUCache
{
public:
    explicit LRUCache(int mSize = 32, bool lock = false):maxSize(mSize), useLock(lock), deleter(nullptr), h(nullptr), t(nullptr){}

    template<typename Deleter>
    explicit LRUCache(Deleter deleter, int mSize = 32, bool lock = false):maxSize(mSize), useLock(lock), h(nullptr), t(nullptr)
    {
        this->deleter = new CustomDeleter<Deleter>(deleter);
    }
    ~LRUCache() { if(deleter) delete deleter; }

    bool contains(const K &key)
    {
        return hash.find(key) != hash.end();
    }

    void put(const K &key, const V &value)
    {
        MutexLocker lock(this->lock, useLock);
        Node *node;
        auto iter = hash.find(key);
        if(iter!=hash.end())
        {
            node=&iter.value();
            node->value=value;
            take(node);
        }
        else
        {
            Node n;
            n.value=value;
            auto nIter = hash.insert(key, n);
            node = &nIter.value();
            node->key=&nIter.key();
        }
        prepend(node);
        if(hash.count()>maxSize) clean();
    }
    V &refVal(const K &key)
    {
        MutexLocker lock(this->lock, useLock);
        return hash[key].value;
    }
    V get(const K &key)
    {
        MutexLocker lock(this->lock, useLock);
        auto iter = hash.find(key);
        if(iter==hash.end()) return V();
        Node *node = &iter.value();
        if(node==h) return node->value;
        take(node);
        prepend(node);
        return node->value;
    }
    void remove(const K &key, bool useDeleter=true)
    {
        MutexLocker lock(this->lock, useLock);
        auto iter = hash.find(key);
        if(iter==hash.end()) return;
        Node *node = &iter.value();
        if(deleter && useDeleter)
        {
            if(!deleter->exec(node->value)) return;
        }
        take(node);
        hash.remove(key);
    }
private:
    struct MutexLocker
    {
        QMutex &m;
        bool locked;
        MutexLocker(QMutex &mutex, bool lock=true):m(mutex), locked(lock){if(lock) m.lock();}
        ~MutexLocker() { if(locked) m.unlock(); }
    private:
        MutexLocker(MutexLocker &);
    };

    struct CustomData
    {
        virtual bool exec(V val) = 0;
    };
    template<typename Deleter>
    struct CustomDeleter : public CustomData
    {
        Deleter d;
        CustomDeleter(Deleter deleter):d(deleter){}
        bool exec(V val){return d(val);}
    };

    int maxSize;
    QMutex lock{QMutex::Recursive};
    bool useLock;
    CustomData *deleter;

    struct Node
    {
        Node():p(nullptr),n(nullptr),key(nullptr){}
        Node *p, *n;
        const K *key;
        V value;
    };
    QHash<K, Node> hash;
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
    void clean()
    {
        int i=maxSize/2;
        Node *cur=t;
        while(i>0 && cur)
        {
            Node *n=cur->p;
            if((deleter && deleter->exec(cur->value)) || !deleter)
            {
                if(n) n->n=nullptr;
                hash.remove(*cur->key);
            } else {
                take(cur);
                prepend(cur);
            }
            cur=n;
            --i;
        }
        t=cur;
    }
};

#endif // LRUCACHE_H
