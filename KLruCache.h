#pragma once 

#include <cstring>
#include <list>
#include <memory>
#include <mutex>
#include <unordered_map>

#include "KICachePolicy.h"

namespace KamaCache
{

// 前向声明
template<typename Key, typename Value> class KLruCache;

/*
LruNode是核心数据结构，存储缓存中最基本的数据
key 键， value 值
还有访问次数 accessCount
此外还有两个指针，这个两个指针是用来构建双向量表的，分别是prev和next
这两个指针在双向链表中的用处是，用来维护一个有序的缓存列表，有序依赖前面的访问次数。
为了避免循环引用，这里使用了weak_ptr来代替普通的shared_ptr，因为weak_ptr不会增加对象的引用计数，它只会增加对象的弱引用计数。

在接口上还提供了getkey getvalue获取键值数据，这样是为了保持封装性，保护内部数据不被随意修改。
同时也保证了扩展性，便于未来修改内部实现而不影响外部调用。
*/
template<typename Key, typename Value>
class LruNode 
{
private:
    Key key_;
    Value value_;
    size_t accessCount_;  // 访问次数
    std::weak_ptr<LruNode<Key, Value>> prev_;  // 改为weak_ptr打破循环引用
    std::shared_ptr<LruNode<Key, Value>> next_;

public:
    LruNode(Key key, Value value)
        : key_(key)
        , value_(value)
        , accessCount_(1) 
    {}

    // 提供必要的访问器
    Key getKey() const { return key_; }
    Value getValue() const { return value_; }
    void setValue(const Value& value) { value_ = value; }
    size_t getAccessCount() const { return accessCount_; }
    void incrementAccessCount() { ++accessCount_; }

    friend class KLruCache<Key, Value>;
    /*
    这里使用了友元类，KLruCache是LruNode的友元类，这样KLruCache就可以访问LruNode的私有成员变量
    两者逻辑高度相关，可以互相访问私有变量成员
    虽然牺牲了一定的封装性，但换来了高效率和清晰的职责划分
    */
};


template<typename Key, typename Value>
class KLruCache : public KICachePolicy<Key, Value>
{
public:
    using LruNodeType = LruNode<Key, Value>;
    using NodePtr = std::shared_ptr<LruNodeType>;
    using NodeMap = std::unordered_map<Key, NodePtr>;

    KLruCache(int capacity)
        : capacity_(capacity)
    {
        initializeList();
    }

    ~KLruCache() override = default;

    // 添加缓存
    void put(Key key, Value value) override
    {
        if (capacity_ <= 0)
            return;
    
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = nodeMap_.find(key);
        if (it != nodeMap_.end())
        {
            // 如果在当前容器中,则更新value,并调用get方法，代表该数据刚被访问
            updateExistingNode(it->second, value);
            return ;
        }

        addNewNode(key, value);
    }

    bool get(Key key, Value& value) override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = nodeMap_.find(key);
        if (it != nodeMap_.end())
        {
            moveToMostRecent(it->second);
            value = it->second->getValue();
            return true;
        }
        return false;
    }

    Value get(Key key) override
    {
        Value value{};
        // memset(&value, 0, sizeof(value));   // memset 是按字节设置内存的，对于复杂类型（如 string）使用 memset 可能会破坏对象的内部结构
        get(key, value);
        return value;
    }

    // 删除指定元素
    void remove(Key key) 
    {   
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = nodeMap_.find(key);
        if (it != nodeMap_.end())
        {
            removeNode(it->second);
            nodeMap_.erase(it);
        }
    }

private:
    void initializeList()
    {
        // 创建首尾虚拟节点
        dummyHead_ = std::make_shared<LruNodeType>(Key(), Value());
        dummyTail_ = std::make_shared<LruNodeType>(Key(), Value());
        dummyHead_->next_ = dummyTail_;
        dummyTail_->prev_ = dummyHead_;
    }

    void updateExistingNode(NodePtr node, const Value& value) 
    {
        node->setValue(value);
        moveToMostRecent(node);
    }

    void addNewNode(const Key& key, const Value& value) 
    {
       if (nodeMap_.size() >= capacity_) 
       {
           evictLeastRecent();
       }

       NodePtr newNode = std::make_shared<LruNodeType>(key, value);
       insertNode(newNode);
       nodeMap_[key] = newNode;
    }

    // 将该节点移动到最新的位置
    void moveToMostRecent(NodePtr node) 
    {
        removeNode(node);
        insertNode(node);
    }

    void removeNode(NodePtr node) 
    {
        if(!node->prev_.expired() && node->next_) 
        {
            auto prev = node->prev_.lock(); // 使用lock()获取shared_ptr
            prev->next_ = node->next_;
            node->next_->prev_ = prev;
            node->next_ = nullptr; // 清空next_指针，彻底断开节点与链表的连接
        }
    }

    // 从尾部插入结点
    void insertNode(NodePtr node) 
    {
        node->next_ = dummyTail_;
        node->prev_ = dummyTail_->prev_;
        dummyTail_->prev_.lock()->next_ = node; // 使用lock()获取shared_ptr
        dummyTail_->prev_ = node;
    }

    // 驱逐最近最少访问
    void evictLeastRecent() 
    {
        NodePtr leastRecent = dummyHead_->next_;
        removeNode(leastRecent);
        nodeMap_.erase(leastRecent->getKey());
    }

private:
    // 缓存容量， 这里规定了缓存的上限，即最大条目数，当缓存到达该容量时，触发淘汰机制
    // 为什么这么做呢？为了防止内存无限制的增长，后续也可以更具资源调整缓存大小
    int           capacity_; 
    NodeMap       nodeMap_; // key -> Node 
    std::mutex    mutex_; // 这个是用来保证线程安全的，保证数据一致性
    /*
    双向链表，
    */
    NodePtr       dummyHead_; // 虚拟头结点
    NodePtr       dummyTail_;
};

// LRU优化：Lru-k版本。 通过继承的方式进行再优化
template<typename Key, typename Value>
class KLruKCache : public KLruCache<Key, Value>
{
public:
    KLruKCache(int capacity, int historyCapacity, int k)
        : KLruCache<Key, Value>(capacity) // 调用基类构造
        , historyList_(std::make_unique<KLruCache<Key, size_t>>(historyCapacity))
        , k_(k)
    {}

    Value get(Key key) 
    {
        // 首先尝试从主缓存获取数据
        Value value{};
        bool inMainCache = KLruCache<Key, Value>::get(key, value);

        // 获取并更新访问历史计数
        size_t historyCount = historyList_->get(key);
        historyCount++;
        historyList_->put(key, historyCount);

        // 如果数据在主缓存中，直接返回
        if (inMainCache) 
        {
            return value;
        }

        // 如果数据不在主缓存，但访问次数达到了k次
        if (historyCount >= k_) 
        {
            // 检查是否有历史值记录
            auto it = historyValueMap_.find(key);
            if (it != historyValueMap_.end()) 
            {
                // 有历史值，将其添加到主缓存
                Value storedValue = it->second;
                
                // 从历史记录移除
                historyList_->remove(key);
                historyValueMap_.erase(it);
                
                // 添加到主缓存
                KLruCache<Key, Value>::put(key, storedValue);
                
                return storedValue;
            }
            // 没有历史值记录，无法添加到缓存，返回默认值
        }

        // 数据不在主缓存且不满足添加条件，返回默认值
        return value;
    }

    void put(Key key, Value value) 
    {
        // 检查是否已在主缓存
        Value existingValue{};
        bool inMainCache = KLruCache<Key, Value>::get(key, existingValue);
        
        if (inMainCache) 
        {
            // 已在主缓存，直接更新
            KLruCache<Key, Value>::put(key, value);
            return;
        }
        
        // 获取并更新访问历史
        size_t historyCount = historyList_->get(key);
        historyCount++;
        historyList_->put(key, historyCount);
        
        // 保存值到历史记录映射，供后续get操作使用
        historyValueMap_[key] = value;
        
        // 检查是否达到k次访问阈值
        if (historyCount >= k_) 
        {
            // 达到阈值，添加到主缓存
            historyList_->remove(key);
            historyValueMap_.erase(key);
            KLruCache<Key, Value>::put(key, value);
        }
    }

private:
    int                                     k_; // 进入缓存队列的评判标准
    std::unique_ptr<KLruCache<Key, size_t>> historyList_; // 访问数据历史记录(value为访问次数)
    std::unordered_map<Key, Value>          historyValueMap_; // 存储未达到k次访问的数据值
};

// lru优化：对lru进行分片，提高高并发使用的性能
template<typename Key, typename Value>
class KHashLruCaches
{
public:
    KHashLruCaches(size_t capacity, int sliceNum)
        : capacity_(capacity)
        , sliceNum_(sliceNum > 0 ? sliceNum : std::thread::hardware_concurrency())
    {
        size_t sliceSize = std::ceil(capacity / static_cast<double>(sliceNum_)); // 获取每个分片的大小
        for (int i = 0; i < sliceNum_; ++i)
        {
            lruSliceCaches_.emplace_back(new KLruCache<Key, Value>(sliceSize)); 
        }
    }

    void put(Key key, Value value)
    {
        // 获取key的hash值，并计算出对应的分片索引
        size_t sliceIndex = Hash(key) % sliceNum_;
        lruSliceCaches_[sliceIndex]->put(key, value);
    }

    bool get(Key key, Value& value)
    {
        // 获取key的hash值，并计算出对应的分片索引
        size_t sliceIndex = Hash(key) % sliceNum_;
        return lruSliceCaches_[sliceIndex]->get(key, value);
    }

    Value get(Key key)
    {
        Value value;
        memset(&value, 0, sizeof(value));
        get(key, value);
        return value;
    }

private:
    // 将key转换为对应hash值
    size_t Hash(Key key)
    {
        std::hash<Key> hashFunc;
        return hashFunc(key);
    }

private:
    size_t                                              capacity_;  // 总容量
    int                                                 sliceNum_;  // 切片数量
    std::vector<std::unique_ptr<KLruCache<Key, Value>>> lruSliceCaches_; // 切片LRU缓存
};

} // namespace KamaCache