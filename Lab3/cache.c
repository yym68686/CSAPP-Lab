#include "common.h"

void mem_read(uintptr_t block_num, uint8_t *buf);
void mem_write(uintptr_t block_num, const uint8_t *buf);

// cache slot
typedef struct {
	// 标记
	uint32_t tag;
	// 有效位
	uint8_t valid;
	// 脏位
	uint8_t modified;
	// 数据块
	uint8_t data[BLOCK_SIZE];
} CacheSlot;

// 每组行数
int line_of_group;
// cache组数
int group_count;
// 组id长度
int group_id_width;
// tag长度
int tag_width;
// cache
CacheSlot * * cache;

// 从cache中读出 addr 地址处的4字节数据
// 若缺失, 需要先从内存中读入数据
uint32_t cache_read(uintptr_t addr) {
	try_increase(1);
	uint32_t val;
	uint32_t offset;
	uint32_t group_id;
	uint32_t tag;

	// addr align 4
	addr = addr & ~0x3;
	// 相对块起始位置的偏移量
	offset = addr & ~(~0 << BLOCK_WIDTH);
	group_id = (addr >> 6) & ~(~0 << group_id_width);
	tag = (addr >> (6 + group_id_width)) & ~(~0 << tag_width);
	int i, j;
	for (i = 0;i < line_of_group;i++)
		// 判断有效位和标记
		if (cache[group_id][i].valid && cache[group_id][i].tag == tag) {
			hit_increase(1);
			val = 0;
			for (j = 3;j >= 0;j--) {
				val *= 0x100;
				val += cache[group_id][i].data[offset + j];
			}
			return val;
		}
	// 跳出循环说明命中失败
	// 遍历该组的行尝试寻找未使用的行
	for (i = 0;i < line_of_group;i++)
		if (!cache[group_id][i].valid) {
			// 从内存读取数据到cache
			mem_read(addr >> BLOCK_WIDTH, cache[group_id][i].data);
			cache[group_id][i].tag = addr >> (BLOCK_WIDTH + group_id_width);
			cache[group_id][i].valid = 1;
			cache[group_id][i].modified = 0;
			val = 0;
			for (j = 3;j >= 0;j--) {
				val *= 0x100;
				val += cache[group_id][i].data[offset + j];
			}
			return val;
		}
	// 未找到空闲行则进行随机替换
	i = rand() % line_of_group;
	// 被替换的cache行的脏位为1则需先写回到内存中
	if (cache[group_id][i].modified == 1)
		mem_write((cache[group_id][i].tag << group_id_width) + group_id, cache[group_id][i].data);
	// 从内存读取数据到cache
	mem_read(addr >> BLOCK_WIDTH, cache[group_id][i].data);
	cache[group_id][i].tag = addr >> (BLOCK_WIDTH + group_id_width);
	cache[group_id][i].valid = 1;
	cache[group_id][i].modified = 0;
	val = 0;
	for (j = 3;j >= 0;j--) {
		val *= 0x100;
		val += cache[group_id][i].data[offset + j];
	}
	return val;
}

// 往cache中addr地址所属的块写入数据 data , 写掩码为 wmask 
// 例如当 wmask 为 0xff 时, 只写入低8比特
// 若缺失, 需要从先内存中读入数据
void cache_write(uintptr_t addr, uint32_t data, uint32_t wmask) {
	try_increase(1);
	uint32_t offset;
	uint32_t group_id;
	uint32_t tag;
	uint32_t * ptr;

	// addr align 4
	addr = addr & ~0x3;
	// 相对块起始位置的偏移量
	offset = addr & ~(~0 << BLOCK_WIDTH);
	group_id = (addr >> 6) & ~(~0 << group_id_width);
	tag = (addr >> (6 + group_id_width)) & ~(~0 << tag_width);
	int i;
	for (i = 0;i < line_of_group;i++)
		// 判断有效位和标记
		if (cache[group_id][i].valid && cache[group_id][i].tag == tag) {
			hit_increase(1);
			cache[group_id][i].modified = 1;
			ptr = (uint32_t *)&cache[group_id][i].data[offset];
			*ptr = (*ptr & ~wmask) | (data & wmask);
			return;
		}
	// 跳出循环说明命中失败
	// 遍历该组的行尝试寻找未使用的行

	for (i = 0;i < line_of_group;i++)
		// 从内存读取数据到cache
		if (!cache[group_id][i].valid) {
			mem_read(addr >> BLOCK_WIDTH, cache[group_id][i].data);
			cache[group_id][i].tag = addr >> (BLOCK_WIDTH + group_id_width);
			cache[group_id][i].valid = 1;
			cache[group_id][i].modified = 1;
			ptr = (uint32_t *)&cache[group_id][i].data[offset];
			*ptr = (*ptr & ~wmask) | (data & wmask);
			return;
		}
	// 未找到空闲行则进行随机替换
	i = rand() % line_of_group;
	// 被替换的cache行的脏位为1则需先写回到内存中
	if (cache[group_id][i].modified == 1)
		mem_write((cache[group_id][i].tag << group_id_width) + group_id, cache[group_id][i].data);
	// 从内存读取数据到cache
	mem_read(addr >> BLOCK_WIDTH, cache[group_id][i].data);
	cache[group_id][i].tag = addr >> (BLOCK_WIDTH + group_id_width);
	cache[group_id][i].valid = 1;
	cache[group_id][i].modified = 1;
	ptr = (uint32_t *)&cache[group_id][i].data[offset];
	*ptr = (*ptr & ~wmask) | (data & wmask);
	return;
}

// 初始化一个数据大小为 2^total_size_width B, 关联度为 2^associativity_width 的cache
// 例如 init_cache(14, 2) 将初始化一个16KB, 4路组相联的cache
// 将所有valid bit置为无效即可
void init_cache(int total_size_width, int associativity_width) {
	line_of_group = exp2(associativity_width);
	group_count = exp2(total_size_width) / BLOCK_SIZE / line_of_group;
	group_id_width = total_size_width - associativity_width - BLOCK_WIDTH;
	tag_width = 32 - BLOCK_SIZE - group_id_width;
	// 32bit
	cache = (CacheSlot * *)malloc(sizeof(CacheSlot *) * group_count);
	int i;
	// 清零
	for (i = 0;i < group_count;i++)
		cache[i] = (CacheSlot *)calloc(line_of_group, sizeof(CacheSlot));
	return;
}
