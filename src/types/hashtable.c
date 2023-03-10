#include <types.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
struct HashMap {
	uint64_t size; //!< entries in the map
	uint64_t capacity; //!< maximum possible map size 
	struct pair_t * pairs; //!< array of pairs
	void * shkey; //!< spihash key
};

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
		const uint64_t hashv = (uint64_t)map->pairs[i].first;
		if(!map->pairs[i].first) continue;
		entries[hashv % (map->capacity << 1)] = map->pairs[hashv % map->capacity];
	}
	
	free(map->pairs);

	map->pairs = entries;
	map->capacity <<= 1;

	return 0;
}

struct HashMap * create_map(uint64_t prealloc) {
	static char key[16] = {0};
	struct HashMap * map = calloc(1, sizeof(struct HashMap));

	if (!*key) {
		srand(time(NULL));
		for (int i = 0; i < 16; i++)
			key[i] = rand();
	}

	if (!map) return NULL;
	if (!prealloc) prealloc = 32;

	map->capacity = prealloc;
	map->size = 0;
	map->pairs = calloc(prealloc, sizeof(struct pair_t));
	map->shkey = key;

	if (!map->pairs) {
		free(map);
		return NULL;
	}

	return map;
}

int map_addi_key(struct HashMap * map, uint64_t key, void * value) {
	if (map->pairs[key % map->capacity].first == (void*)key) {
		map->pairs[key % map->capacity].second = value;
		return key;
	}

	++map->size;

	if (map->pairs[key % map->capacity].first == NULL) 
	{
		map->pairs[key % map->capacity].first = (void*)key;
		map->pairs[key % map->capacity].second = value;
		return key;
	}

	while(map->pairs[key % map->capacity].first != NULL) {
		if (map_rebuffer(map)) return -1;
	}

	map->pairs[key % map->capacity].first = (void *)key;
	map->pairs[key % map->capacity].second = value;
	return key;
}

int map_adds_key(struct HashMap * map, const char * key, void * value) {
	return map_addi_key(map, siphash(key, strlen(key), map->shkey), value);
}

int map_addss_key(struct HashMap * map, struct string_t key, void * value) {
	return map_addi_key(map, siphash(key.string, key.size, map->shkey), value);
}

struct pair_t *map_getkey_i(const struct HashMap * map, const uint64_t key) {
	return &map->pairs[key % map->capacity];
}

struct pair_t *map_getkey_s(const struct HashMap * map, const char * key) {
	return &map->pairs[siphash(key, strlen(key), map->shkey) % map->capacity];
}

struct pair_t *map_getkey_ss(const struct HashMap * map, const struct string_t key) {
	return &map->pairs[siphash(key.string, key.size, map->shkey) % map->capacity];
}

void delete_map(struct HashMap * map, deleter_func deleter) {
	for (size_t i = 0; i < map->capacity; i++)
	{
		if(!map->pairs[i].first) continue;
		if (deleter && map->pairs[i].second) deleter(map->pairs[i].second);
	}

	free(map->pairs);
	free(map);
}
