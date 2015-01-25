CC=clang
CFLAGS=-Wall -pedantic -g -fPIC -I includes
LIBS=-lm -lreadline 

TARGET	= libjl777.a
SRCS	= picoc.c table.c lex.c parse.c expression.c heap.c type.c \
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
	ar rcu  libs/libjl777.a  $(OBJS) gzip/*.o libtom/*.o #libs/randombytes.o;\

test:	all
	(cd tests; make test)

clean: doesntexist
	rm -f libjl777.a libs/libjl777.so $(OBJS) *~

SuperNET: $(TARGET); \
    pkill SuperNET; rm SuperNET; gcc -o SuperNET SuperNET.c libs/libminiupnpc.a libs/libjl777.a libs/libwebsockets.a libs/libuv.a libs/libdb.a -lssl -lcrypto -lpthread -lcurl -lm

special: /usr/lib/libjl777.so; \
    gcc -shared -Wl,-soname,libjl777.so -o libs/libjl777.so $(OBJS) -lstdc++ -lc -lcurl -lm -ldl; \
    sudo cp libs/libjl777.so /usr/lib/libjl777.so

btcd: ../src/BitcoinDarkd; \
    cd ../src; rm BitcoinDarkd; make -f makefile.unix; strip BitcoinDarkd; cp BitcoinDarkd ../libjl777

btcdmac: ../src/BitcoinDarkd; \
    cd ../src; rm BitcoinDarkd; make -f makefile.osx; strip BitcoinDarkd; cp BitcoinDarkd ../libjl777

install: doesntexist; \
    sudo aptitude install python-software-properties software-properties-common autotools-dev ; add-apt-repository ppa:bitcoin/bitcoin; echo "deb http://ppa.launchpad.net/webupd8team/java/ubuntu trusty main" | tee /etc/apt/sources.list.d/webupd8team-java.list ; echo "deb-src http://ppa.launchpad.net/webupd8team/java/ubuntu trusty main" | tee -a /etc/apt/sources.list.d/webupd8team-java.list ; apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 --recv-keys EEA14886 ; aptitude update; aptitude install git build-essential libdb++-dev  libtool  autoconf pkg-config libssl-dev libboost-all-dev libminiupnpc-dev clang libcurl4-gnutls-dev oracle-java8-installer libwebsockets3 libwebsockets-dev cmake

patch: doesntexist; \
    cd miniupnpc; \
    make libminiupnpc.a; \
    cp libminiupnpc.a ../libs; \
    cd ..;\

ramchains: doesntexist; \
    mkdir ramchains; \
    cd ramchains; \
    unzip ../ramchains.zip; \
    cd ..;\

patch1: doesntexist; \
   export LIBDIR="/usr/local/BerkeleyDB.6.1/lib"; \
    unzip db-6.1.19.zip; \
    cd db-6.1.19/build_unix; \
    ../dist/configure; \
    cp ../../env_region.c ../src/env; \
    make; \
    cp *.h ../../includes; \
    cp libdb.a ../../libs;

patch2: doesntexist; \
    unzip lws.zip -d libwebsockets; \
    cd libwebsockets/lib; \
    cmake ..; \
    cp libwebsockets.h lws_config.h ../../includes; \
    cp libwebsockets-test-server.key.pem ../../SuperNET.key.pem; \
    cp libwebsockets-test-server.pem ../../SuperNET.pem; \
    make; \
    cp lib/*  ../../libs; \
    cd ../..;

onetime: doesntexist; \
    cd miniupnpc; \
    make; \
    cp libminiupnpc.a ../libs; \
    cd ..;\
    unzip lws.zip -d libwebsockets; \
    cd libwebsockets/lib; \
    cmake ..; \
    cp libwebsockets.h lws_config.h ../../includes; \
    cp libwebsockets-test-server.key.pem ../../SuperNET.key.pem; \
    cp libwebsockets-test-server.pem ../../SuperNET.pem; \
    make; \
    cp lib/*  ../../libs; \
    cd ../..; \
    unzip db-6.1.19.zip; \
    cd db-6.1.19/build_unix; \
    ../dist/configure; \
    cp ../../env_region.c ../src/env; \
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

count:
	@echo "Core:"
	@cat $(SRCS) *.h | grep -v '^[ 	]*/\*' | grep -v '^[ 	]*$$' | wc
	@echo ""
	@echo "Everything:"
	@cat $(SRCS) *.h */*.h | wc

.PHONY: libjl777.c

../src/BitcoinDarkd: libjl777.a
#libs/randombytes.o:
libs/libuv.a:
/usr/lib/libjl777.so: libs/libjl777.so
doesntexist:
picoc.o: picoc.c picoc.h
libgfshare.o: libgfshare.c libgfshare.h
libjl777.o: libjl777.c atomic.h ciphers.h feeds.h jl777hash.h libgfshare.h mofnfs.h packets.h sorts.h tradebot.h \
            storage.h bars.h cJSON.h jl777str.h NXTservices.h telepathy.h transporter.h \
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

