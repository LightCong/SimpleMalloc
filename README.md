# SimpleMalloc

小内存分配管理器

## 简介

SimpleMalloc是一个针对于小内存（小于256字节）的内存分配管理器。
使用c语言实现。
实现上参考了python源码，与SGI STL中二级内存分配器的设计。

## 详细设计

SimpleMalloc 是一个具有多层次缓存的内存分配器。
在SimpleMalloc中，内存角色按照尺寸从小到大分别是:block,pool,pond。

block 是用户可以获取并自由访问的内存区域。block的大小总是8字节的整数倍,这样做是出于内存对齐的考虑。？

pool 是block的集合，相当于block的容器，每一个pool的尺寸是4k字节，即为一个内存页的大小，这可以加速系统级内存分配。

此外每一个pool的起始地址一定是pool尺寸的整数倍。这样有助于通过block的地址迅速定位block所在的pool。

最后pond 的大小是256k字节，作为pool的容器，每个pond 包含了 63个pool，这是因为pond_header 和pool内存对齐的影响。

详细设计图如下:

![内存结构图](https://github.com/LightCong/SimpleMalloc/blob/master/pic/p1.png)

## 使用教程

writing

## TODO LIST

1. 压力与性能测试
2. 增加对mmap的支持

## DONE LIST

1. makefile
