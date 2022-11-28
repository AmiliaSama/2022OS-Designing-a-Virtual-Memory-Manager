#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#define TLB_SIZE 16
#define PAGES 256
// PAGES-1，8位1辅助变量
#define PAGE_MASK 255
#define PAGE_SIZE 256
#define OFFSET_BITS 8
// 同8位1
#define OFFSET_MASK 255
// 内存大小
#define MEMORY_SIZE PAGES *PAGE_SIZE

// addresses 输入最大长度
#define BUFFER_SIZE 10

struct node {
    unsigned char logical;
    unsigned char physical;
};

// 用循环数组存储作为tlb的数据结构，当数组满时会覆盖
struct node tlb[TLB_SIZE];

// 已经加入到tlb数组中的
int tlbindex = 0;

// 页表项，-1表示不存在（需要初始化）
int pagetable[PAGES];

// 内存总大小
signed char main_memory[MEMORY_SIZE];

// 一个指针，用以对应内存在backing_store中的位置
signed char *backing;

int max(int a, int b) { return a > b ? a : b; }

// 添加tlb，如果满了需要覆盖最先存储的那一项
void add_tlb(unsigned char logical, unsigned char physical) {
    struct node *entry = &tlb[tlbindex % TLB_SIZE];
    tlbindex++;
    entry->logical = logical;
    entry->physical = physical;
}

// 查找tlb，找到返回物理地址，否则返回-1表示TLB miss
int search_tlb(unsigned char logical_page) {
    for (int i = max((tlbindex - TLB_SIZE), 0); i < tlbindex; i++) {
        struct node *entry = &tlb[i % TLB_SIZE];
        if (entry->logical == logical_page) {
            return entry->physical;
        }
    }

    return -1;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        puts("命令参数缺失，需要BACKING_STORE和addresses.txt");
        exit(1);
    }

    char *backing_filename = argv[1];
    int backing_fd = open(backing_filename, O_RDONLY);
    backing = mmap(0, MEMORY_SIZE, PROT_READ, MAP_PRIVATE, backing_fd, 0);

    char *input_filename = argv[2];
    FILE *input_fp = fopen(input_filename, "r");

    // 初始化为-1表示未占用
    for (int i = 0; i < PAGES; i++) {
        pagetable[i] = -1;
    }
    // buffer缓存
    char buffer[BUFFER_SIZE];

    // 待输出的参数
    int total_addresses = 0;
    int tlb_hits = 0;
    int page_faults = 0;
    // 在内存中尚未分配的页表空间序号，用unsigned表示0~255
    unsigned char free_page = 0;
    while (fgets(buffer, BUFFER_SIZE, input_fp) != NULL) {
        total_addresses++;
        int logical_address = atoi(buffer);
        int offset =
            logical_address & OFFSET_MASK; // 和255与（其它位置是0），得到偏移)
        int logical_page = (logical_address >> OFFSET_BITS) & PAGE_MASK;
        int physical_page = search_tlb(logical_page);

        // 找到了，TLB hit
        if (physical_page != -1) {
            tlb_hits++;

        }
        // 没找到，TLB miss
        else {
            physical_page = pagetable[logical_page];

            // Page fault
            if (physical_page == -1) {
                page_faults++;

                physical_page = free_page;
                free_page++;

                // 把backing_store中的数据存入
                memcpy(main_memory + physical_page * PAGE_SIZE,
                       backing + logical_page * PAGE_SIZE, PAGE_SIZE);

                pagetable[logical_page] = physical_page;
            }

            // 记得把tlb加上
            add_tlb(logical_page, physical_page);
        }
        // 找到物理地址：页表的offset
        //(页地址+偏移地址就是物理地址)
        int physical_address = (physical_page << OFFSET_BITS) | offset;
        // 查找对应value
        signed char value = main_memory[physical_page * PAGE_SIZE + offset];
        printf("Virtual address: %d Physical address: %d Value: %d\n",
               logical_address, physical_address, value);
    }

    printf("Total number of Translated Addresses = %d\n", total_addresses);
    printf("Page Faults = %d, Page Fault Rate = %.3f\n", page_faults,
           page_faults / (1. * total_addresses));
    printf("TLB Hits = %d, TLB Hit Rate = %.3f\n", tlb_hits,
           tlb_hits / (1. * total_addresses));

    return 0;
}