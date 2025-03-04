#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // for getopt()

#define BYTES_PER_WORD 4
// #define DEBUG

/*
 * Cache structures
 */
int time = 0;

typedef struct
{
	int age;
	int valid;
	int modified;
	uint32_t tag;
} cline;

typedef struct
{
	cline *lines;
} cset;

typedef struct
{
	int s;
	int E;
	int b;
	cset *sets;
} cache;

// void xdump(cache *L);

static int index_bit(int n)
{
	int cnt = 0;
	while (n)
	{
		cnt++;
		n = n >> 1;
	}
	return cnt - 1;
}

int bit_size(int n)
{
	int key = 1;
	int result = 0;

	while (key < n)
	{
		key *= 2;
		result += 1;
	}

	return result;
}

int bit_max(int n)
{
	int result = 0;
	int key = 1;
	int i = 0;

	while (i < n)
	{
		result += key;
		key *= 2;
		i += 1;
	}

	return result;
}

void increase_age(cache *L)
{
	int i = 0;
	int j = 0;

	for (i = 0; i < (1 << L->s); i++)
	{
		for (j = 0; j < L->E; j++)
		{
			L->sets[i].lines[j].age += 1;
		}
	}
}

int run_LRU(cache *L, int index)
{
	int key = L->sets[index].lines[0].age;
	int result = 0;
	int i = 0;

	for (i = 0; i < L->E; i++)
	{
		if (key < L->sets[index].lines[i].age)
		{
			key = L->sets[index].lines[i].age;
			result = i;
		}
	}

	return result;
}

cache build_cache(int set, int way, int blocksize)
{
	cache c;

	int i = 0;
	int j = 0;

	c.E = way;
	c.s = bit_size(set);
	c.b = bit_size(blocksize);

	c.sets = malloc(sizeof(cset) * set);

	for (i = 0; i < set; i++)
	{
		c.sets[i].lines = malloc(sizeof(cline) * way);
		for (j = 0; j < way; j++)
		{
			c.sets[i].lines[j].age = 0;
			c.sets[i].lines[j].valid = 0;
			c.sets[i].lines[j].modified = 0;
			c.sets[i].lines[j].tag = 0;
		}
	}

	return c;
}

int access_cache(cache L, char *op, uint32_t addr)
{
	int index = (addr >> L.b) & bit_max(L.s);
	int tag = addr >> (L.b + L.s);
	int way = L.E;

	int targetWayIdx = -1;
	int result = 0; // miss, hit, writeback

	int i = 0;

	increase_age(&L);

	while (i < way)
	{
		int targetValid = L.sets[index].lines[i].valid;
		int targetTag = L.sets[index].lines[i].tag;
		int targetAge = L.sets[index].lines[i].age;

		switch (targetValid)
		{
		case 0: // valid == 0: miss
			if (targetWayIdx < 0)
			{
				targetWayIdx = i;
			}
			break;
		case 1:
			if (targetTag == tag) // hit
			{
				if (*op == 'W') // store operation
				{
					L.sets[index].lines[i].modified = 1;
				}

				L.sets[index].lines[i].age = 0;
				targetWayIdx = -1;

				return 1;
			}
		}

		i += 1;
	}

	if (targetWayIdx >= 0) // miss: it can be added.
	{
		if (L.sets[index].lines[targetWayIdx].modified)
		{
			result = 2;
		}

		L.sets[index].lines[targetWayIdx].valid = 1;
		L.sets[index].lines[targetWayIdx].tag = tag;
		L.sets[index].lines[targetWayIdx].age = 0;
		L.sets[index].lines[targetWayIdx].modified = 0;
	}
	else // miss: all occupied.
	{
		targetWayIdx = run_LRU(&L, index); // LRU

		if (L.sets[index].lines[targetWayIdx].modified)
		{
			result = 2;
		}

		L.sets[index].lines[targetWayIdx].valid = 1;
		L.sets[index].lines[targetWayIdx].tag = tag;
		L.sets[index].lines[targetWayIdx].age = 0;
		L.sets[index].lines[targetWayIdx].modified = 0;
	}

	if (*op == 'W') // store operation
	{
		L.sets[index].lines[targetWayIdx].modified = 1;
	}

	return result;
}

/***************************************************************/
/*                                                             */
/* Procedure : cdump                                           */
/*                                                             */
/* Purpose   : Dump cache configuration                        */
/*                                                             */
/***************************************************************/
void cdump(int capacity, int assoc, int blocksize)
{

	printf("Cache Configuration:\n");
	printf("-------------------------------------\n");
	printf("Capacity: %dB\n", capacity);
	printf("Associativity: %dway\n", assoc);
	printf("Block Size: %dB\n", blocksize);
	printf("\n");
}

/***************************************************************/
/*                                                             */
/* Procedure : sdump                                           */
/*                                                             */
/* Purpose   : Dump cache stat		                             */
/*                                                             */
/***************************************************************/
void sdump(int total_reads, int total_writes, int write_backs,
					 int reads_hits, int write_hits, int reads_misses, int write_misses)
{
	printf("Cache Stat:\n");
	printf("-------------------------------------\n");
	printf("Total reads: %d\n", total_reads);
	printf("Total writes: %d\n", total_writes);
	printf("Write-backs: %d\n", write_backs);
	printf("Read hits: %d\n", reads_hits);
	printf("Write hits: %d\n", write_hits);
	printf("Read misses: %d\n", reads_misses);
	printf("Write misses: %d\n", write_misses);
	printf("\n");
}

/***************************************************************/
/*                                                             */
/* Procedure : xdump                                           */
/*                                                             */
/* Purpose   : Dump current cache state                        */
/* 					                            		                   */
/* Cache Design						                                     */
/*  							                                             */
/* 	     cache[set][assoc][word per block]		                 */
/*                                						                 */
/*      				                        		                   */
/*       -------------------------------------	               */
/*       I        I  way0  I  way1  I  way2  I                 */
/*       -------------------------------------                 */
/*       I        I  word0 I  word0 I  word0 I                 */
/*       I  set0  I  word1 I  word1 I  work1 I                 */
/*       I        I  word2 I  word2 I  word2 I                 */
/*       I        I  word3 I  word3 I  word3 I                 */
/*       -------------------------------------                 */
/*       I        I  word0 I  word0 I  word0 I                 */
/*       I  set1  I  word1 I  word1 I  work1 I                 */
/*       I        I  word2 I  word2 I  word2 I                 */
/*       I        I  word3 I  word3 I  word3 I                 */
/*       -------------------------------------                 */
/*                              						       						 */
/*                                                             */
/***************************************************************/
void xdump(cache *L)
{
	int i, j, k = 0;
	int b = L->b, s = L->s;
	int way = L->E, set = 1 << s;
	int E = index_bit(way);

	uint32_t line;

	printf("Cache Content:\n");
	printf("-------------------------------------\n");
	for (i = 0; i < way; i++)
	{
		if (i == 0)
		{
			printf("    ");
		}
		printf("      WAY[%d]", i);
	}
	printf("\n");

	for (i = 0; i < set; i++)
	{
		printf("SET[%d]:   ", i);
		for (j = 0; j < way; j++)
		{
			if (k != 0 && j == 0)
			{
				printf("          ");
			}
			if (L->sets[i].lines[j].valid)
			{
				line = L->sets[i].lines[j].tag << (s + b);
				line = line | (i << b);
			}
			else
			{
				line = 0;
			}
			printf("0x%08x  ", line);
		}
		printf("\n");
	}
	printf("\n");
}

int main(int argc, char *argv[])
{
	int i, j, k;
	int capacity = 1024;
	int way = 8;
	int blocksize = 8;
	int set;

	//cache
	cache simCache;

	// counts
	int read = 0, write = 0, writeback = 0;
	int readhit = 0, writehit = 0;
	int readmiss = 0, writemiss = 0;

	// Input option
	int opt = 0;
	char *token;
	int xflag = 0;

	// parse file
	char *trace_name = (char *)malloc(32);
	FILE *fp;
	char line[16];
	char *op;
	uint32_t addr;

	/* You can define any variables that you want */
	int access_result = 0;

	trace_name = argv[argc - 1];
	if (argc < 3)
	{
		printf("Usage: %s -c cap:assoc:block_size [-x] input_trace \n", argv[0]);
		exit(1);
	}
	while ((opt = getopt(argc, argv, "c:x")) != -1)
	{
		switch (opt)
		{
		case 'c':
			// extern char *optarg;
			token = strtok(optarg, ":");
			capacity = atoi(token);
			token = strtok(NULL, ":");
			way = atoi(token);
			token = strtok(NULL, ":");
			blocksize = atoi(token);
			break;
		case 'x':
			xflag = 1;
			break;
		default:
			printf("Usage: %s -c cap:assoc:block_size [-x] input_trace \n", argv[0]);
			exit(1);
		}
	}

	// allocate
	set = capacity / way / blocksize;

	/* TODO: Define a cache based on the struct declaration */
	simCache = build_cache(set, way, blocksize);

	// simulate
	fp = fopen(trace_name, "r"); // read trace file
	if (fp == NULL)
	{
		printf("\nInvalid trace file: %s\n", trace_name);
		return 1;
	}
	cdump(capacity, way, blocksize);

	/* TODO: Build an access function to load and store data from the file */
	while (fgets(line, sizeof(line), fp) != NULL)
	{
		op = strtok(line, " ");
		addr = strtoull(strtok(NULL, ","), NULL, 16);

#ifdef DEBUG
		// You can use #define DEBUG above for seeing traces of the file.
		fprintf(stderr, "op: %s\n", op);
		fprintf(stderr, "addr: %x\n", addr);
#endif

		access_result = access_cache(simCache, op, addr);

		switch (*op)
		{
		case 'R':
			read += 1;
			switch (access_result)
			{
			case 2:
				writeback += 1;
			case 0:
				readmiss += 1;
				break;
			case 1:
				readhit += 1;
				break;
			}
			break;
		case 'W':
			write += 1;
			switch (access_result)
			{
			case 2:
				writeback += 1;
			case 0:
				writemiss += 1;
				break;
			case 1:
				writehit += 1;
				break;
			}
		}
	}

	// test example
	sdump(read, write, writeback, readhit, writehit, readmiss, writemiss);
	if (xflag)
	{
		xdump(&simCache);
	}

	return 0;
}
