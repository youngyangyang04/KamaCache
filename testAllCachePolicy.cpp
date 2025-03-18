#include <iostream>
#include <string>
#include <chrono>
#include <vector>
#include <iomanip>
#include <random>
#include <algorithm>
#include <array>

#include "KICachePolicy.h"
#include "KLfuCache.h"
#include "KLruCache.h"
#include "KArcCache/KArcCache.h"

class Timer {
public:
    Timer() : start_(std::chrono::high_resolution_clock::now()) {}
    
    double elapsed() {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(now - start_).count();
    }

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start_;
};

// 辅助函数：打印结果
void printResults(const std::string& testName, int capacity, 
                 const std::vector<int>& get_operations, 
                 const std::vector<int>& hits) {
    std::cout << "缓存大小: " << capacity << std::endl;
    std::cout << "LRU - 命中率: " << std::fixed << std::setprecision(2) 
              << (100.0 * hits[0] / get_operations[0]) << "%" << std::endl;
    std::cout << "LFU - 命中率: " << std::fixed << std::setprecision(2) 
              << (100.0 * hits[1] / get_operations[1]) << "%" << std::endl;
    std::cout << "ARC - 命中率: " << std::fixed << std::setprecision(2) 
              << (100.0 * hits[2] / get_operations[2]) << "%" << std::endl;
}

void testHotDataAccess() {
    std::cout << "\n=== 测试场景1：热点数据访问测试 ===" << std::endl;
    
    const int CAPACITY = 50;  // 增加缓存容量
    const int OPERATIONS = 500000;  // 增加操作次数
    const int HOT_KEYS = 20;   // 增加热点数据的数量
    const int COLD_KEYS = 5000;        
    
    KamaCache::KLruCache<int, std::string> lru(CAPACITY);
    KamaCache::KLfuCache<int, std::string> lfu(CAPACITY);
    KamaCache::KArcCache<int, std::string> arc(CAPACITY);

    std::random_device rd;
    std::mt19937 gen(rd());
    
    std::array<KamaCache::KICachePolicy<int, std::string>*, 3> caches = {&lru, &lfu, &arc};
    std::vector<int> hits(3, 0);
    std::vector<int> get_operations(3, 0);

    // 先进行一系列put操作
    for (int i = 0; i < caches.size(); ++i) {
        for (int op = 0; op < OPERATIONS; ++op) {
            int key;
            if (op % 100 < 70) {  // 70%热点数据
                key = gen() % HOT_KEYS;
            } else {  // 30%冷数据
                key = HOT_KEYS + (gen() % COLD_KEYS);
            }
            std::string value = "value" + std::to_string(key);
            caches[i]->put(key, value);
        }
        
        // 然后进行随机get操作
        for (int get_op = 0; get_op < OPERATIONS; ++get_op) {
            int key;
            if (get_op % 100 < 70) {  // 70%概率访问热点
                key = gen() % HOT_KEYS;
            } else {  // 30%概率访问冷数据
                key = HOT_KEYS + (gen() % COLD_KEYS);
            }
            
            std::string result;
            get_operations[i]++;
            if (caches[i]->get(key, result)) {
                hits[i]++;
            }
        }
    }

    printResults("热点数据访问测试", CAPACITY, get_operations, hits);
}

void testLoopPattern() {
    std::cout << "\n=== 测试场景2：循环扫描测试 ===" << std::endl;
    
    const int CAPACITY = 50;  // 增加缓存容量
    const int LOOP_SIZE = 500;         
    const int OPERATIONS = 200000;  // 增加操作次数
    
    KamaCache::KLruCache<int, std::string> lru(CAPACITY);
    KamaCache::KLfuCache<int, std::string> lfu(CAPACITY);
    KamaCache::KArcCache<int, std::string> arc(CAPACITY);

    std::array<KamaCache::KICachePolicy<int, std::string>*, 3> caches = {&lru, &lfu, &arc};
    std::vector<int> hits(3, 0);
    std::vector<int> get_operations(3, 0);

    std::random_device rd;
    std::mt19937 gen(rd());

    // 先填充数据
    for (int i = 0; i < caches.size(); ++i) {
        for (int key = 0; key < LOOP_SIZE; ++key) {  // 只填充 LOOP_SIZE 的数据
            std::string value = "loop" + std::to_string(key);
            caches[i]->put(key, value);
        }
        
        // 然后进行访问测试
        int current_pos = 0;
        for (int op = 0; op < OPERATIONS; ++op) {
            int key;
            if (op % 100 < 60) {  // 60%顺序扫描
                key = current_pos;
                current_pos = (current_pos + 1) % LOOP_SIZE;
            } else if (op % 100 < 90) {  // 30%随机跳跃
                key = gen() % LOOP_SIZE;
            } else {  // 10%访问范围外数据
                key = LOOP_SIZE + (gen() % LOOP_SIZE);
            }
            
            std::string result;
            get_operations[i]++;
            if (caches[i]->get(key, result)) {
                hits[i]++;
            }
        }
    }

    printResults("循环扫描测试", CAPACITY, get_operations, hits);
}

void testWorkloadShift() {
    std::cout << "\n=== 测试场景3：工作负载剧烈变化测试 ===" << std::endl;
    
    const int CAPACITY = 4;            
    const int OPERATIONS = 80000;      
    const int PHASE_LENGTH = OPERATIONS / 5;
    
    KamaCache::KLruCache<int, std::string> lru(CAPACITY);
    KamaCache::KLfuCache<int, std::string> lfu(CAPACITY);
    KamaCache::KArcCache<int, std::string> arc(CAPACITY);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::array<KamaCache::KICachePolicy<int, std::string>*, 3> caches = {&lru, &lfu, &arc};
    std::vector<int> hits(3, 0);
    std::vector<int> get_operations(3, 0);

    // 先填充一些初始数据
    for (int i = 0; i < caches.size(); ++i) {
        for (int key = 0; key < 1000; ++key) {
            std::string value = "init" + std::to_string(key);
            caches[i]->put(key, value);
        }
        
        // 然后进行多阶段测试
        for (int op = 0; op < OPERATIONS; ++op) {
            int key;
            // 根据不同阶段选择不同的访问模式
            if (op < PHASE_LENGTH) {  // 热点访问
                key = gen() % 5;
            } else if (op < PHASE_LENGTH * 2) {  // 大范围随机
                key = gen() % 1000;
            } else if (op < PHASE_LENGTH * 3) {  // 顺序扫描
                key = (op - PHASE_LENGTH * 2) % 100;
            } else if (op < PHASE_LENGTH * 4) {  // 局部性随机
                int locality = (op / 1000) % 10;
                key = locality * 20 + (gen() % 20);
            } else {  // 混合访问
                int r = gen() % 100;
                if (r < 30) {
                    key = gen() % 5;
                } else if (r < 60) {
                    key = 5 + (gen() % 95);
                } else {
                    key = 100 + (gen() % 900);
                }
            }
            
            std::string result;
            get_operations[i]++;
            if (caches[i]->get(key, result)) {
                hits[i]++;
            }
            
            // 随机进行put操作，更新缓存内容
            if (gen() % 100 < 30) {  // 30%概率进行put
                std::string value = "new" + std::to_string(key);
                caches[i]->put(key, value);
            }
        }
    }

    printResults("工作负载剧烈变化测试", CAPACITY, get_operations, hits);
}

int main() {
    testHotDataAccess();
    testLoopPattern();
    testWorkloadShift();
    return 0;
}