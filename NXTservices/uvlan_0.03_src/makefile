VER	= 0.03
CFLAGS += -Wall -W -O3 -I./ -I../../libtomcrypt/src/headers -I../../libtomcrypt -I../../libtomcrypt/testprof -L../../libtomcrypt -L../../libtommath -DLTM_DESC -DLTC_EASY

OBJECTS = crypto.o tree.o jshtml.o xmlp.o main.o

default: uvlan

ifndef CRYPT
CRYPT=-ltomcrypt -ltommath
endif

uvlan: ${OBJECTS}
	$(CC) $(CFLAGS) ${OBJECTS} -lpcap -lpthread $(CRYPT) -o uvlan

zipup:
	rm -f uvlan_*_linux.* uvlan_*_win32.*
	zip -9 uvlan_$(VER)_linux.zip etc/uvlan_sample.xml uvlan
	tar czvf uvlan_$(VER)_linux.tar.gz etc/uvlan_sample.xml uvlan
	zip -9 uvlan_$(VER)_src.zip *.c *.h makefile etc/uvlan_sample.xml LICENSE README
	tar czvf uvlan_$(VER)_src.tar.gz *.c *.h makefile etc/uvlan_sample.xml LICENSE README

clean:
	rm -f ${OBJECTS} uvlan

