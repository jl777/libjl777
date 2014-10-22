CC=clang
CFLAGS=-Wall -pedantic -g -fPIC -I includes
LIBS=-lm -lreadline 

TARGET	= libjl777.a
SRCS	= envglue.c picoc.c table.c lex.c parse.c expression.c heap.c type.c \
	variable.c clibrary.c platform.c include.c \
	platform/platform_unix.c platform/library_unix.c \
	cstdlib/stdio.c cstdlib/math.c cstdlib/string.c cstdlib/stdlib.c \
	cstdlib/time.c cstdlib/errno.c cstdlib/ctype.c cstdlib/stdbool.c \
	cstdlib/unistd.c libgfshare.c libjl777.c gzip/adler32.c gzip/crc32.c gzip/gzclose.c \
    gzip/gzread.c gzip/infback.c  gzip/inflate.c   gzip/trees.c    gzip/zutil.c gzip/compress.c  gzip/deflate.c \
    gzip/gzlib.c gzip/gzwrite.c  gzip/inffast.c  gzip/inftrees.c  gzip/uncompr.c libtom/yarrow.c\
    libtom/aes.c libtom/cast5.c libtom/khazad.c   libtom/rc2.c     libtom/safer.c      libtom/skipjack.c \
    libtom/aes_tab.c libtom/crypt_argchk.c  libtom/kseed.c    libtom/rc5.c     libtom/saferp.c     libtom/twofish.c \
    libtom/anubis.c libtom/des.c libtom/multi2.c   libtom/rc6.c     libtom/safer_tab.c  libtom/twofish_tab.c \
    libtom/blowfish.c libtom/kasumi.c  libtom/noekeon.c  libtom/rmd160.c  libtom/sha256.c     libtom/xtea.c
OBJS	:= $(SRCS:%.c=%.o)

all: $(TARGET)

$(TARGET): $(OBJS)
  	#$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LIBS)
	ar rcu  libs/libjl777.a  $(OBJS) gzip/*.o libtom/*.o libs/randombytes.o;\
    gcc -shared -Wl,-soname,libjl777.so -o libs/libjl777.so $(OBJS) libs/libdb.a -lstdc++ -lc -lcurl -lm -ldl
    
test:	all
	(cd tests; make test)

clean: doesntexist
	rm -f libjl777.a libs/libjl777.so $(OBJS) *~

libtest: $(TARGET); \
    rm libtest; gcc -o libtest libtest.c libs/libjl777.a libs/libdb.a libs/libuv.a -lpthread -lcurl -lm

install: /usr/lib/libjl777.so; \
    sudo cp libs/libjl777.so /usr/lib/libjl777.so

btcd: ../src/BitcoinDarkd; \
    cd ../src; rm BitcoinDarkd; make -f makefile.unix; strip BitcoinDarkd; cp BitcoinDarkd ../libjl777

onetime: doesntexist; \
    unzip db-6.1.19.zip; \
    cd db-6.1.19/build_unix; \
    ../dist/configure; \
    make; \
    cp libdb.a ../../libs; \
    cp *.h ../../includes; \
    cd ../..; \
    cd libuv; \
    sh autogen.sh; \
    ./configure; \
    make; \
    cp .libs/libuv.a ../libs; \
    cp .libs/libuv.so ../libs; \
    cd ..; \
    echo "expanding nacl"; \
    bzip2 -dc < nacl-20090405.tar.bz2 | tar -xf -; \
    cd nacl-20090405; \
    echo "compiling nacl, this will take some time"; \
    ./do; \
    cd ..; \
    echo "randombytes.o and libnacl.a are in the build directory of nacl-20090405"; \
    echo `date`; \
    echo `ls -l nacl-20090405/build/*/lib/amd64/randombytes.o`; \
    cp nacl-20090405/build/*/lib/amd64/randombytes.o libs

count:
	@echo "Core:"
	@cat $(SRCS) *.h | grep -v '^[ 	]*/\*' | grep -v '^[ 	]*$$' | wc
	@echo ""
	@echo "Everything:"
	@cat $(SRCS) *.h */*.h | wc

.PHONY: libjl777.c

../src/BitcoinDarkd: libjl777.a
libs/randombytes.o:
libs/libuv.a:
/usr/lib/libjl777.so: libs/libjl777.so
doesntexist:
picoc.o: picoc.c picoc.h
libgfshare.o: libgfshare.c libgfshare.h
libjl777.o: libjl777.c atomic.h ciphers.h feeds.h jl777hash.h libgfshare.h mofnfs.h packets.h sorts.h tradebot.h \
            storage.h bars.h cJSON.h jl777str.h NXTservices.h peers.h telepathy.h transporter.h \
            bitcoind.h coincache.h jdatetime.h jsoncodec.h NXTutils.h sortnetworks.h telepods.h \
            bitcoinglue.h coins.h jl777.h kademlia.h mappedptr.h orders.h _sorts.h teleport.h udp.h tweetnacl.h
table.o: table.c interpreter.h platform.h
lex.o: lex.c interpreter.h platform.h
parse.o: parse.c picoc.h interpreter.h platform.h
expression.o: expression.c interpreter.h platform.h
heap.o: heap.c interpreter.h platform.h
type.o: type.c interpreter.h platform.h
variable.o: variable.c interpreter.h platform.h
clibrary.o: clibrary.c picoc.h interpreter.h platform.h
platform.o: platform.c picoc.h interpreter.h platform.h
include.o: include.c picoc.h interpreter.h platform.h
platform/platform_unix.o: platform/platform_unix.c picoc.h interpreter.h platform.h
platform/library_unix.o: platform/library_unix.c interpreter.h platform.h
cstdlib/stdio.o: cstdlib/stdio.c interpreter.h platform.h
cstdlib/math.o: cstdlib/math.c interpreter.h platform.h
cstdlib/string.o: cstdlib/string.c interpreter.h platform.h
cstdlib/stdlib.o: cstdlib/stdlib.c interpreter.h platform.h
cstdlib/time.o: cstdlib/time.c interpreter.h platform.h
cstdlib/errno.o: cstdlib/errno.c interpreter.h platform.h
cstdlib/ctype.o: cstdlib/ctype.c interpreter.h platform.h
cstdlib/stdbool.o: cstdlib/stdbool.c interpreter.h platform.h
cstdlib/unistd.o: cstdlib/unistd.c interpreter.h platform.h

