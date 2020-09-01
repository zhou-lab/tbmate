CC = gcc
CFLAGS = -W -Wall -finline-functions -fPIC -std=gnu99 -Wno-unused-result -O3
CLIB = -lncurses -lpthread -lz -lm
CF_OPTIMIZE = 1

OS := $(shell uname)
ifeq ($(OS),  Darwin)
	CFLAGS += -Wno-unused-function
else
	CLIB += -lrt -ltinfo
endif

INCLUDE = include

PROG = tbmate

.PHONY: build
build: exportcf $(PROG)

exportcf:
	$(eval export CF_OPTIMIZE)

.PHONY: debug
debug: CF_OPTIMIZE := 0
debug: CFLAGS += -g -pg
debug: CFLAGS := $(filter-out -O3,$(CFLAGS))
debug: build

#####################
##### libraries #####
#####################

LHTSLIB_DIR = htslib
LHTSLIB_INCLUDE = htslib/htslib
LHTSLIB = $(LHTSLIB_DIR)/libhts.a
$(LHTSLIB) :
	make -C $(LHTSLIB_DIR) libhts.a

###################
### subcommands ###
###################

pack.o: pack.c
	$(CC) -c $(CFLAGS) -I$(LUTILS_DIR) -I$(LHTSLIB_INCLUDE) $< -o $@

view.o: view.c
	$(CC) -c $(CFLAGS) -I$(LUTILS_DIR) -I$(LHTSLIB_INCLUDE) $< -o $@

chunk.o: chunk.c
	$(CC) -c $(CFLAGS) -I$(LUTILS_DIR) -I$(LHTSLIB_INCLUDE) $< -o $@

header.o: header.c
	$(CC) -c $(CFLAGS) -I$(LUTILS_DIR) -I$(LHTSLIB_INCLUDE) $< -o $@

LIBS=view.o chunk.o pack.o header.o $(LHTSLIB)

tbmate: $(LIBS) main.c
	gcc $(CFLAGS) main.c -o $@ $(LIBS) $(CLIB)


## clean just src
.PHONY: clean
clean :
	rm -f *.o tbmate
	make -C $(LHTSLIB_DIR) clean
