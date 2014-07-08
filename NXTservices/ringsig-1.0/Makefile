C_FILES = crypto-ops-data.c crypto-ops.c crypto.c hash.c keccak.c random.c

H_FILES = crypto-ops.h crypto.h hash-ops.h hash.h keccak.h random.h

DEFS = -D_POSIX_C_SOURCE=200809L

#C_FLAGS = -std=c99 -Wall -g -ggdb3
C_FLAGS = -std=c99 -Wall -O2

all: run_tests ringsig

run_tests: $(C_FILES) $(H_FILES) run_tests.c
	gcc $(C_FLAGS) $(DEFS) crypto-ops.c crypto-ops-data.c hash.c keccak.c run_tests.c -o run_tests
	
ringsig: $(C_FILES) $(H_FILES) main.c
	gcc $(C_FLAGS) $(DEFS) $(C_FILES) main.c -o ringsig

clean:
	rm -f run_tests ringsig
