#pragma once

#include "../KICachePolicy.h"
#include "KArcLruPart.h"
#include "KArcLfuPart.h"
#include <memory>

namespace KamaCache 
{

template<typename Key, typename Value>
class KArcCache : public KICachePolicy<Key, Value> 
{
public:
    explicit KArcCache(size_t capacity = 10, size_t transformThreshold = 2)
        : capacity_(capacity)
        , transformThreshold_(transformThreshold)
        , lruPart_(std::make_unique<ArcLruPart<Key, Value>>(capacity, transformThreshold))
        , lfuPart_(std::make_unique<ArcLfuPart<Key, Value>>(capacity, transformThreshold))
    {}

    ~KArcCache() override = default;

    void put(const Key& key, const Value& value) override
    {
        checkGhostCaches(key);

        // 检查 LFU 部分是否存在该键
        bool inLfu = lfuPart_->contain(key);
        // 更新 LRU 部分缓存
        lruPart_->put(key, value);
        // 如果 LFU 部分存在该键，则更新 LFU 部分
        if (inLfu) 
        {
            lfuPart_->put(key, value);
        }
    }

    bool get(const Key& key, Value& value) override
    {
        checkGhostCaches(key);

        bool shouldTransform = false;
        if (lruPart_->get(key, value, shouldTransform)) 
        {
            if (shouldTransform) 
            {
                lfuPart_->put(key, value);
            }
            return true;
        }
        return lfuPart_->get(key, value);
    }

    Value get(const Key& key) override
    {
        Value value{};
        get(key, value);
        return value;
    }

private:
    bool checkGhostCaches(const Key& key)
    {
        bool inGhost = false;
        if (lruPart_->checkGhost(key)) 
        {
            if (lfuPart_->decreaseCapacity()) 
            {
                lruPart_->increaseCapacity();
            }
            inGhost = true;
        } 
        else if (lfuPart_->checkGhost(key)) 
        {
            if (lruPart_->decreaseCapacity()) 
            {
                lfuPart_->increaseCapacity();
            }
            inGhost = true;
        }
        return inGhost;
    }

private:
    size_t capacity_;
    size_t transformThreshold_;
    std::unique_ptr<ArcLruPart<Key, Value>> lruPart_;
    std::unique_ptr<ArcLfuPart<Key, Value>> lfuPart_;
};

} // namespace KamaCache