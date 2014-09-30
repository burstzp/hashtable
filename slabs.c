#include<stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>

#define POWER_SMALLEST 3
#define POWER_LARGEST  20
#define POWER_BLOCK 1048576

typedef unsigned int uint;
typedef unsigned long ulong;

typedef struct {
    uint size;
    uint perslab;

    void *end_page_ptr;
    uint end_page_free;

    uint slabs;
    void **slab_list;
    uint list_size;

    void **slots;           /* list of item ptrs */
    unsigned int sl_total;  /* size of previous array */
    unsigned int sl_curr;   /* first free slot */
} slabclass_t;

static slabclass_t slabclass[POWER_LARGEST+1];
static unsigned int mem_limit = 0;
static unsigned int mem_malloced = 0;

void *shm_alloc(uint size) {
    int fd;

    fd = open("/dev/zero", O_RDWR);
    if (fd == -1) {
        return NULL;
    }
    
    void *addr = (void *) mmap(NULL, size, 
            PROT_READ | PROT_WRITE,
            MAP_SHARED, fd, 0);

    close(fd);

    return addr;
}

uint slabs_clsid(unsigned int size) {
    int res = 1;

    if(size==0)
        return 0;
    size--;
    while(size >>= 1)
        res++;
    if (res < POWER_SMALLEST) 
        res = POWER_SMALLEST;
    if (res > POWER_LARGEST)
        res = 0;
    return res;
}

void slabs_init(uint limit) {
    int i;
    int size = 1;
    mem_limit += limit;
    for(i=0; i<=POWER_LARGEST; i++, size*=2) {
        slabclass[i].size = size;
        slabclass[i].perslab = POWER_BLOCK / size;
        slabclass[i].end_page_ptr = 0;
        slabclass[i].end_page_free = 0;
        slabclass[i].slabs = 0;
        slabclass[i].slab_list = 0;
        slabclass[i].list_size = 0;
    }
}

static int grow_slab_list (unsigned int id) { 
    slabclass_t *p = &slabclass[id];

    if (p->slabs == p->list_size) {
        unsigned int new_size =  p->list_size ? p->list_size * 2 : 16;
        void *new_list = realloc(p->slab_list, new_size*sizeof(void*));
        if (new_list == 0) return 0;
        p->list_size = new_size;
        p->slab_list = new_list;
    }
    return 1;
}

int slabs_newslab(unsigned int id) {
    slabclass_t *p = &slabclass[id];
    int num = p->perslab;
    int len = POWER_BLOCK;
    char *ptr;

    if (mem_limit && mem_malloced + len > mem_limit)
        return 0;

    if (!grow_slab_list(id)) return 0;

    ptr = shm_alloc(len);
    if (ptr == 0) return 0;

    memset(ptr, 0, len);
    p->end_page_ptr = ptr;
    p->end_page_free = num;

    p->slab_list[p->slabs++] = ptr;
    mem_malloced += len;
    return 1;
}

void *slabs_alloc(uint size) {
    slabclass_t *p;

    unsigned char id = slabs_clsid(size);
    if (id < POWER_SMALLEST || id > POWER_LARGEST)
        return 0;
    p = &slabclass[id];
    if (!(p->end_page_ptr || p->sl_curr || slabs_newslab(id))) {
        return 0;
    }

    /* return off our freelist, if we have one */
    if (p->sl_curr)
        return p->slots[--p->sl_curr];
    
    /* if we recently allocated a whole page, return from that */
    if (p->end_page_ptr) {
        void *ptr = p->end_page_ptr;
        if (--p->end_page_free) {
            p->end_page_ptr += p->size;
        } else {
            p->end_page_ptr = 0;
        }
        return ptr;
    }

    return 0;
}

void slabs_free(void *ptr, unsigned int size) {
    unsigned char id = slabs_clsid(size);
    slabclass_t *p;
    if (id < POWER_SMALLEST || id > POWER_LARGEST)
        return;

    p = &slabclass[id];
    
    if (p->sl_curr == p->sl_total) { 
        int new_size = p->sl_total ? p->sl_total*2 : 16;  /* 16 is arbitrary */
        void **new_slots = realloc(p->slots, new_size*sizeof(void *));
        if (new_slots == 0)
            return;
        p->slots = new_slots;
        p->sl_total = new_size;
    }
    p->slots[p->sl_curr++] = ptr; 
}

char *slabs_stats(int *buflen) {
    int i, total;
    char *buf = (char*) malloc(8192);
    char *bufcurr = buf;

    *buflen = 0;
    if (!buf) return 0;

    total = 0;
    for(i = POWER_SMALLEST; i <= POWER_LARGEST; i++) {
        slabclass_t *p = &slabclass[i];
        if (p->slabs) {
            unsigned int perslab, slabs;

            slabs = p->slabs;
            perslab = p->perslab;

            bufcurr += sprintf(bufcurr, "STAT %d:chunk_size %u\r\n", i, p->size);
            bufcurr += sprintf(bufcurr, "STAT %d:chunks_per_page %u\r\n", i, perslab);
            bufcurr += sprintf(bufcurr, "STAT %d:total_pages %u\r\n", i, slabs);
            bufcurr += sprintf(bufcurr, "STAT %d:total_chunks %u\r\n", i, slabs*perslab);
            bufcurr += sprintf(bufcurr, "STAT %d:used_chunks %u\r\n", i, slabs*perslab - p->sl_curr);
            bufcurr += sprintf(bufcurr, "STAT %d:free_chunks %u\r\n", i, p->sl_curr);
            bufcurr += sprintf(bufcurr, "STAT %d:free_chunks_end %u\r\n", i, p->end_page_free);
            total++;
        }
    }
    bufcurr += sprintf(bufcurr, "STAT active_slabs %d\r\nSTAT total_malloced %u", total, mem_malloced);
    *buflen = bufcurr - buf;

    return buf;
}

#ifdef DEBUG
int main(void) {
    slabs_init(10240000);
    char *s = slabs_alloc(10);
    strcpy(s, "fuckddddddddkklklkld");
    int len;
    printf("a = %s", slabs_stats(&len));

}
#endif
