#ifndef LRUCACHE_H
#define LRUCACHE_H
#include <QHash>
#include <QRecursiveMutex>
#include <QDateTime>
#include <QTimer>
#include <QTimerEvent>
#include <QSharedPointer>
template <typename K, typename V>
class LRUCache
{
public:
    using Deleter = std::function<bool (V)>;
    explicit LRUCache(const char *name, size_t mSize = 32, bool dynamicSize = false, bool lock = false)
        : cacheName(name), maxSize(mSize), useLock(lock), dynamicAdjust(dynamicSize), dynamicMaxSize(mSize),
          cleanTimestamp(0), deleter(nullptr), h(nullptr), t(nullptr)
    {
        if(mSize < 2) maxSize = dynamicMaxSize = 2;
    }
    explicit LRUCache(const char *name, Deleter deleter, int mSize = 32, bool dynamicSize = false, bool lock = false)
        : cacheName(name), maxSize(mSize), useLock(lock), dynamicAdjust(dynamicSize), dynamicMaxSize(mSize),
          cleanTimestamp(0), h(nullptr), t(nullptr)
    {
        if(mSize < 2) maxSize = dynamicMaxSize = 2;
        this->deleter = QSharedPointer<Deleter>::create(deleter);
    }

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
        if(hash.count()>dynamicMaxSize) clean();
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
            if(!(*deleter)(node->value)) return;
        }
        take(node);
        hash.remove(key);
    }
private:
    struct MutexLocker
    {
        QRecursiveMutex &m;
        bool locked;
        MutexLocker(QRecursiveMutex &mutex, bool lock=true):m(mutex), locked(lock){if(lock) m.lock();}
        ~MutexLocker() { if(locked) m.unlock(); }
    private:
        MutexLocker(MutexLocker &);
    };

    const char *cacheName = "";
    size_t maxSize, dynamicMaxSize;
    QRecursiveMutex lock;
    bool useLock;
    bool dynamicAdjust;
    qint64 cleanTimestamp;
    QSharedPointer<Deleter> deleter;

    struct Node
    {
        Node():p(nullptr),n(nullptr),key(nullptr){}
        Node *p, *n;
        const K *key;
        V value;
    };
    QMap<K, Node> hash;
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
        if(dynamicAdjust)
        {
            qint64 ts = QDateTime::currentDateTime().toMSecsSinceEpoch();
            if (ts - cleanTimestamp < 1000 && dynamicMaxSize < maxSize * 4)
            {
                dynamicMaxSize <<= 1;
                qInfo("Cache[%s] Adjust To: %d", cacheName, dynamicMaxSize);
                return;
            }
            cleanTimestamp = ts;
        }
        size_t i = dynamicMaxSize >> 1;
        Node *cur=t;
        while(i>0 && cur)
        {
            Node *n=cur->p;
            if((deleter && (*deleter)(cur->value)) || !deleter)
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
        qInfo("Cache[%s] Clean, Cache Size: %d, Left: %d", cacheName, dynamicMaxSize, hash.size());
    }
};

template <typename T>
class TimeLimitedCachePool
{
public:
    using Deleter = std::function<void(T)>;

    explicit TimeLimitedCachePool(int interval = 60*1000, Deleter deleter = nullptr) : _deleter(deleter)
    {
        _timer.start(interval);
        QObject::connect(&_timer, &QTimer::timeout, &_timer, [this]() {
            QMutexLocker locker(&_mutex);
            checkExpiredObjects();
        });
    }

    T get()
    {
        QMutexLocker locker(&_mutex);
        if (!_cache.isEmpty()) {
            T object = _cache.first().object;
            _cache.removeFirst();
            return object;
        }
        return T();
    }

    void put(const T &object, int timeout)
    {
        QMutexLocker locker(&_mutex);
        CacheEntry entry;
        entry.object = object;
        entry.expirationTime = QDateTime::currentDateTime().addMSecs(timeout);
        _cache.append(entry);
    }

private:
    void checkExpiredObjects()
    {
        const QDateTime currentTime = QDateTime::currentDateTime();
        auto it = _cache.begin();
        while (it != _cache.end())
        {
            if (it->expirationTime < currentTime)
            {
                if (_deleter) _deleter(it->object);
                it = _cache.erase(it);
            } else {
                ++it;
            }
        }
    }

    struct CacheEntry
    {
        T object;
        QDateTime expirationTime;
    };

    QList<CacheEntry> _cache;
    QTimer _timer;
    QMutex _mutex;
    Deleter _deleter;
};

#endif // LRUCACHE_H
