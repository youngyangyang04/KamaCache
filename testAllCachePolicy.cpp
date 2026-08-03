#include <iostream>
#include <string>
#include <chrono>
#include <vector>
#include <iomanip>
#include <random>
#include <algorithm>
#include <array>
#include "KICachePolicy.h"

#ifdef _WIN32
#include <windows.h>
#endif
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

struct Operation {
    int key;
    bool isPut;
};

// 辅助函数：打印结果
void printResults(const std::string& testName, int capacity,
                  const std::vector<std::string>& names,
                  const std::vector<int>& get_operations,
                  const std::vector<int>& hits) {
    std::cout << "=== " << testName << " 结果汇总 ===" << std::endl;
    std::cout << "缓存大小: " << capacity << std::endl;

    for (size_t i = 0; i < hits.size(); ++i) {
        double hitRate = 100.0 * hits[i] / get_operations[i];
        std::cout << (i < names.size() ? names[i] : "Algorithm " + std::to_string(i + 1))
                  << " - 命中率: " << std::fixed << std::setprecision(2)
                  << hitRate << "% ";
        std::cout << "(" << hits[i] << "/" << get_operations[i] << ")" << std::endl;
    }

    std::cout << std::endl;
}

void testHotDataAccess() {
    std::cout << "\n=== 测试场景1：热点数据访问测试 ===" << std::endl;

    const int CAPACITY = 20;
    const int OPERATIONS = 500000;
    const int HOT_KEYS = 20;
    const int COLD_KEYS = 5000;
    const int SEED = 42;  // 固定种子，确保每次运行结果可复现

    // 预生成操作序列，所有算法共用同一序列保证公平对比
    std::vector<Operation> ops;
    ops.reserve(OPERATIONS);
    std::mt19937 gen(SEED);
    for (int op = 0; op < OPERATIONS; ++op) {
        bool isPut = (gen() % 100 < 30);
        int key;
        if (gen() % 100 < 70) {
            key = gen() % HOT_KEYS;
        } else {
            key = HOT_KEYS + (gen() % COLD_KEYS);
        }
        ops.push_back({key, isPut});
    }

    KamaCache::KLruCache<int, std::string> lru(CAPACITY);
    KamaCache::KLfuCache<int, std::string> lfu(CAPACITY);
    KamaCache::KArcCache<int, std::string> arc(CAPACITY);
    KamaCache::KLruKCache<int, std::string> lruk(CAPACITY, HOT_KEYS + COLD_KEYS, 2);
    KamaCache::KLfuCache<int, std::string> lfuAging(CAPACITY, 20000);

    std::array<KamaCache::KICachePolicy<int, std::string>*, 5> caches = {&lru, &lfu, &arc, &lruk, &lfuAging};
    std::vector<int> hits(5, 0);
    std::vector<int> get_operations(5, 0);
    std::vector<std::string> names = {"LRU", "LFU", "ARC", "LRU-K", "LFU-Aging"};

    // 所有缓存重放相同的操作序列
    for (size_t i = 0; i < caches.size(); ++i) {
        // 预热：插入热点数据
        for (int key = 0; key < HOT_KEYS; ++key) {
            caches[i]->put(key, "value" + std::to_string(key));
        }

        // 重放操作序列
        for (size_t opIdx = 0; opIdx < ops.size(); ++opIdx) {
            const auto& op = ops[opIdx];
            if (op.isPut) {
                std::string value = "value" + std::to_string(op.key) + "_v" + std::to_string(opIdx % 100);
                caches[i]->put(op.key, value);
            } else {
                std::string result;
                get_operations[i]++;
                if (caches[i]->get(op.key, result)) {
                    hits[i]++;
                }
            }
        }
    }

    printResults("热点数据访问测试", CAPACITY, names, get_operations, hits);
}

void testLoopPattern() {
    std::cout << "\n=== 测试场景2：循环扫描测试 ===" << std::endl;

    const int CAPACITY = 50;
    const int LOOP_SIZE = 500;
    const int OPERATIONS = 200000;
    const int SEED = 42;

    // 预生成操作序列
    std::vector<Operation> ops;
    ops.reserve(OPERATIONS);
    std::mt19937 gen(SEED);
    int current_pos = 0;
    for (int op = 0; op < OPERATIONS; ++op) {
        bool isPut = (gen() % 100 < 20);
        int key;
        if (op % 100 < 60) {
            key = current_pos;
            current_pos = (current_pos + 1) % LOOP_SIZE;
        } else if (op % 100 < 90) {
            key = gen() % LOOP_SIZE;
        } else {
            key = LOOP_SIZE + (gen() % LOOP_SIZE);
        }
        ops.push_back({key, isPut});
    }

    KamaCache::KLruCache<int, std::string> lru(CAPACITY);
    KamaCache::KLfuCache<int, std::string> lfu(CAPACITY);
    KamaCache::KArcCache<int, std::string> arc(CAPACITY);
    KamaCache::KLruKCache<int, std::string> lruk(CAPACITY, LOOP_SIZE * 2, 2);
    KamaCache::KLfuCache<int, std::string> lfuAging(CAPACITY, 3000);

    std::array<KamaCache::KICachePolicy<int, std::string>*, 5> caches = {&lru, &lfu, &arc, &lruk, &lfuAging};
    std::vector<int> hits(5, 0);
    std::vector<int> get_operations(5, 0);
    std::vector<std::string> names = {"LRU", "LFU", "ARC", "LRU-K", "LFU-Aging"};

    for (size_t i = 0; i < caches.size(); ++i) {
        // 预热：加载20%的数据
        for (int key = 0; key < LOOP_SIZE / 5; ++key) {
            caches[i]->put(key, "loop" + std::to_string(key));
        }

        // 重放操作序列
        for (size_t opIdx = 0; opIdx < ops.size(); ++opIdx) {
            const auto& op = ops[opIdx];
            if (op.isPut) {
                std::string value = "loop" + std::to_string(op.key) + "_v" + std::to_string(opIdx % 100);
                caches[i]->put(op.key, value);
            } else {
                std::string result;
                get_operations[i]++;
                if (caches[i]->get(op.key, result)) {
                    hits[i]++;
                }
            }
        }
    }

    printResults("循环扫描测试", CAPACITY, names, get_operations, hits);
}

void testWorkloadShift() {
    std::cout << "\n=== 测试场景3：工作负载剧烈变化测试 ===" << std::endl;

    const int CAPACITY = 30;
    const int OPERATIONS = 80000;
    const int PHASE_LENGTH = OPERATIONS / 5;
    const int SEED = 42;

    // 预生成操作序列
    std::vector<Operation> ops;
    ops.reserve(OPERATIONS);
    std::mt19937 gen(SEED);
    for (int op = 0; op < OPERATIONS; ++op) {
        int phase = op / PHASE_LENGTH;

        int putProbability;
        switch (phase) {
            case 0: putProbability = 15; break;
            case 1: putProbability = 30; break;
            case 2: putProbability = 10; break;
            case 3: putProbability = 25; break;
            case 4: putProbability = 20; break;
            default: putProbability = 20;
        }

        bool isPut = (gen() % 100 < putProbability);

        int key;
        if (op < PHASE_LENGTH) {
            key = gen() % 5;
        } else if (op < PHASE_LENGTH * 2) {
            key = gen() % 400;
        } else if (op < PHASE_LENGTH * 3) {
            key = (op - PHASE_LENGTH * 2) % 100;
        } else if (op < PHASE_LENGTH * 4) {
            int locality = (op / 800) % 5;
            key = locality * 15 + (gen() % 15);
        } else {
            int r = gen() % 100;
            if (r < 40) {
                key = gen() % 5;
            } else if (r < 70) {
                key = 5 + (gen() % 45);
            } else {
                key = 50 + (gen() % 350);
            }
        }
        ops.push_back({key, isPut});
    }

    KamaCache::KLruCache<int, std::string> lru(CAPACITY);
    KamaCache::KLfuCache<int, std::string> lfu(CAPACITY);
    KamaCache::KArcCache<int, std::string> arc(CAPACITY);
    KamaCache::KLruKCache<int, std::string> lruk(CAPACITY, 500, 2);
    KamaCache::KLfuCache<int, std::string> lfuAging(CAPACITY, 10000);

    std::array<KamaCache::KICachePolicy<int, std::string>*, 5> caches = {&lru, &lfu, &arc, &lruk, &lfuAging};
    std::vector<int> hits(5, 0);
    std::vector<int> get_operations(5, 0);
    std::vector<std::string> names = {"LRU", "LFU", "ARC", "LRU-K", "LFU-Aging"};

    for (size_t i = 0; i < caches.size(); ++i) {
        // 预热
        for (int key = 0; key < 30; ++key) {
            caches[i]->put(key, "init" + std::to_string(key));
        }

        // 重放操作序列
        for (size_t opIdx = 0; opIdx < ops.size(); ++opIdx) {
            const auto& op = ops[opIdx];
            int phase = static_cast<int>(opIdx) / PHASE_LENGTH;
            if (op.isPut) {
                std::string value = "value" + std::to_string(op.key) + "_p" + std::to_string(phase);
                caches[i]->put(op.key, value);
            } else {
                std::string result;
                get_operations[i]++;
                if (caches[i]->get(op.key, result)) {
                    hits[i]++;
                }
            }
        }
    }

    printResults("工作负载剧烈变化测试", CAPACITY, names, get_operations, hits);
}

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    testHotDataAccess();
    testLoopPattern();
    testWorkloadShift();
    return 0;
}
