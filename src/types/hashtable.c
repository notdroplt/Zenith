#include <types.h>
#include <string.h>
#include <stdlib.h>

struct HashMap {
	uint64_t size; //!< entries in the map
	uint64_t capacity; //!< maximum possible map size 
	struct pair_t * pairs; //!< array of pairs
};


static uint8_t popcount(uint64_t n) {
	uint8_t v = 0;
	for (; n; n >>= 1) if (n & 1) ++v;
	return v;
}
// hash 8-a-time w/ popcount
uint64_t cstr_hash(const char * string) {
	size_t size = strlen(string);
	uint64_t hash = 0x49bf73d903ddeda3;

	while (size % 8) {
		hash ^= (*string << (size % 8)) * 37;
		++string;
		--size;
	}

	// putting the if statement outside makes so its done once
	// not every iteration

	register uint64_t var;
	if (__builtin_cpu_supports ("popcnt"))
		while (size) {
			var = *(uint64_t *)string;
			hash ^= (var * size) << __builtin_popcount(var ^ hash);
			string += 8;
			size -= 8;
		}    
	else 
		while (size) {
			var = *(uint64_t *)string;
			hash ^= (var * size) << popcount(var ^ hash);
			string += 8;
			size -= 8;
		}    

	return hash;
}
/// reallocation happens in two situations:
///	1:	when the size of the buffer is going to be more than 
///		50% of the current size, to avoid  key collison
///
///	2:	when there is a key collision
static int map_rebuffer (struct HashMap * map) {
	struct pair_t * entries = calloc(map->capacity << 1, sizeof(struct pair_t));
	if (!entries) {
		return 1;
	}

	for (size_t i = 0; i < map->capacity; i++)
	{
		if(!map->pairs[i].first) continue;
		const uint64_t hashv = cstr_hash(map->pairs[i].first);
		// keys that didn't collide before will not collide on rebuffering
		entries[hashv % (map->capacity << 1)] = map->pairs[hashv % map->capacity];
	}
	
	free(map->pairs);

	map->pairs = entries;

	return 0;
}

struct HashMap * create_map(uint64_t prealloc) {
	struct HashMap * map = calloc(1, sizeof(struct HashMap));
	if (!map) return NULL;
	if (!prealloc) prealloc = 32;

	map->capacity = prealloc;
	map->size = 0;
	map->pairs = calloc(prealloc, sizeof(struct pair_t));
	if (!map->pairs) {
		free(map);
		return NULL;
	}

	return map;
}

int map_addi_key(struct HashMap * map, uint64_t key, void * value) {
	if (map->pairs[key % map->capacity].first == NULL ||
	    map->pairs[key % map->capacity].first == (void*)key) 
	{
		map->pairs[key % map->capacity].first = (void*)key;
		map->pairs[key % map->capacity].second = value;
		return key;
	}

	while(map->pairs[key % map->capacity].first == NULL) {
		if (map_rebuffer(map)) return 0;
	}

	map->pairs[key % map->capacity].first = (void *)key;
	map->pairs[key % map->capacity].second = value;

	return key;
}