#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include "slabs.h"
#include "hashtable.h"

int main(void) {
    slabs_init(60240000);
    
    FILE *fp = fopen("res.txt", "r");
    HashTable *ht = _create_hashtable(1024);
    int counter = 0;
    while (!feof(fp) ) {
        char buf[1024] = {0};
        fgets(buf,1024,fp);
        char phone[8] = {0};
        char data[100] = {0};
        sscanf(buf, "%s\t%s\n", phone, data);
        if (hash_add(ht, phone, str_hval(data)) == -1) {
            printf("hash_add error key is = %s\n", phone);
            exit(0);
        } 
        counter++;
   //     if (counter % 1000 == 0)printf("counter = %d\n", counter);
    }
    fclose(fp);
    printf("done\n\n");
    //hash_dump_kvstr(ht); 

    Hval *ret = NULL;
    hash_find(ht, "1501006", &ret);
    printf("find-result = %s\n", ret->str.val);
    hash_find(ht, "1363592", &ret);
    printf("find-result = %s\n", ret->str.val);
    hash_find(ht, "1899999", &ret);
    printf("find-result = %s\n", ret->str.val);
}
