main: main.o slabs.o hashtable.o
	gcc $^ -o $@

clean: 
	-rm main *.o

sources=main.c slabs.c hashtable.c
include $(sources:.c=.d)

%.d: %.c 
	set -e; rm -f $@; \
	$(CC) -MM $(CPPFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$
