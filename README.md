# KamaCache

> **本项目目前只在[知识星球](https://programmercarl.com/other/kstar.html)答疑并维护**。

最近很多录友在问：有没有比较小，但又比较完整的项目。

最近[知识星球](https://programmercarl.com/other/kstar.html)里里刚刚发布了 C++版本缓存系统，这个小项目。

代码量不大，只有1100行。

一般的话，每天花 6-8个小时，7天就可以学完。

做这个项目的基础要求：

熟悉C++语法，学会C++11常用特性即可，了解操作系统相关知识，有网络编程经验更佳。

这个项目对于时间紧张的录友比较合适。

**该项目满足一下特点**：

* 代码量不大
* 整体不难
* 又有项目难点可说的
* 可以较快学完

该缓存系统使用多个页面替换策略实现了**一个线程安全的缓存系统**：

* LRU：最近最久未使用页面置换算法
* LFU：最不经常使用页面置换算法
* ARC：自适应替换缓存算法

## 做完本项目，你的收获


* 掌握缓存的作用、层次结构与设计哲学，理解其在系统架构中的关键价值
* 亲手实现并优化LRU、LFU、ARC算法，应对不同业务场景
* 运用互斥锁、原子操作及缓存分片技术，保证多线程数据一致性与高性能
* 通过分片降低锁竞争、预加载预热缓存，显著提升吞吐量与响应速度
* 实践应对缓存穿透、击穿、雪崩的行业通用解决方案
* 从数据结构设计、策略选择到性能评估，获得构建高性能中间件的完整经验

## 什么是缓存？

缓存是将高频访问的数据暂存到内存中，是加速数据访问的存储，降低延迟，提高吞吐率的利器。

## 为什么要实现缓存系统?

因缓存的使用相关需求，**通过牺牲一部分服务器内存，减少对磁盘或者数据库资源进行直接读写，可换取更快响应速度**。

尤其是处理高并发的场景，负责存储经常访问的数据，通过设计合理的缓存机制提高资源的访问效率。

由于服务器的内存是有限的，我们不能把所有数据都存放在内存中，因此需要一种机制来决定当使用内存超过一定标准时，应该删除哪些数据，这就涉及到缓存淘汰策略的选择。

## 实际应用场景

缓存策略在系统和实际业务开发过程中较为常见，下面我为大家列出一些使用了缓存策略的知名系统和组件：

**Linux Kernel**： 使用 LRU 在内存管理中页面缓存(Page Cache)，当物理内存不足时，系统会优先淘汰最近未使用的页面。

**Android内存缓存**：  Android的LRUCache类用于管理有限内存中的应用数据缓存。

**Redis**：Redis 本身并不直接实现 LRU 缓存策略，但它提供了过期和淘汰机制。

在内存不足时，Redis 可以配置不同的淘汰策略，如 volatile-lru（对于设置了过期时间的键，使用 LRU 策略淘汰数据）。

Redis 提供了 LFU 策略（从 4.0 版本开始）作为其内存驱逐策略之一，可以通过 maxmemory-policy 配置。

**PostgreSQL** ：在一些PostgreSQL的缓存拓展中实现了基于ARC的缓存机制

## 缓存系统项目精讲

该项目文档是[知识星球](https://programmercarl.com/other/kstar.html)录友专享的。

项目文档依然是将 「简历写法」给大家列出来了，大家学完就可以参考这个来写简历，分别给出普通写法和进阶写法：

<div align="center"><img src='https://file1.kamacoder.com/i/algo/20241227165531.png' width=500 alt=''></img></div>

做完该项目，面试中大概率会有哪些面试问题，以及如何回答，也列出好了，共27道。

<div align="center"><img src='https://file1.kamacoder.com/i/algo/20241227165754.png' width=500 alt=''></img></div>

文档中的项目面试题都掌握的话，这个项目在面试中基本没问题。

当然项目文档会对本项目代码做详细的讲解：

<div align="center"><img src='https://file1.kamacoder.com/i/algo/20241227170954.png' width=500 alt=''></img></div>

<div align="center"><img src='https://file1.kamacoder.com/i/algo/20241227171014.png' width=500 alt=''></img></div>

<div align="center"><img src='https://file1.kamacoder.com/i/algo/20241227171042.png' width=500 alt=''></img></div>

同时会对三个缓存策略做优劣对对比：

<div align="center"><img src='https://file1.kamacoder.com/i/algo/20241227171143.png' width=500 alt=''></img></div>

同时对缓存命中率做实验效果对比：

<div align="center"><img src='https://file1.kamacoder.com/i/algo/20241227171342.png' width=500 alt=''></img></div>

## 答疑

本项目在[知识星球](https://programmercarl.com/other/kstar.html)里为 文字专栏形式，大家不用担心，看不懂，星球里每个项目有专属答疑群，任何问题都可以在群里问，都会得到解答：

![](https://file1.kamacoder.com/i/web/2025-09-26_11-30-13.jpg)


## 获取本项目专栏

**本文档仅为星球内部专享，大家可以加入[知识星球](https://programmercarl.com/other/kstar.html)里获取，在星球置顶一**

