#ifndef __HASHTABLE_H
#define __HASHTABLE_H

typedef struct _buckets Buckets;
typedef struct _HashTable HashTable;
typedef struct _hval Hval;

typedef unsigned int uint;
typedef unsigned long ulong;

struct _hval {
    long lval;
    double dval;
    struct {
        char *val;
        int len;
    } str;
};

struct _buckets {
    char *key;
    Hval *val;
    Buckets *left, *right;
    Buckets *list_left, *list_right;
};

struct _HashTable {
    char *ktype;
    char *vtype;
    int tableSize;
    int tableMask;
    ulong numberOffElements;
    Buckets *tail, *head;
    Buckets *cur;
    Buckets **arrBuckets;
};
HashTable *_create_hashtable(uint size);
int hash_find(HashTable *ht, const char *key, Hval **ret);
int hash_add(HashTable *ht, const char *key, Hval *val);
int hash_del(HashTable *ht, char *key);
ulong numberOfElements(HashTable *ht);

inline Hval *long_hval(long val);
inline Hval *double_hval(double val);
inline Hval *str_hval(const char *val);

typedef void (*fp)(HashTable *ht);
inline void hash_dump_kvint(HashTable *ht);
inline void hash_dump_kvstr(HashTable *ht);
inline void hash_dump_ksvi(HashTable *ht);
inline void hash_dump_kivs(HashTable *ht);
inline void hash_dump(HashTable *ht, fp p);

#endif
