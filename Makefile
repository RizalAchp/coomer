# coomer - C zoomer
# See LICENSE file for copyright and license details.
.POSIX:
COOMER_VERSION=0.1.0
PKG_INCS=`pkg-config --cflags xext` \
		 `pkg-config --cflags x11` \
		 `pkg-config --cflags xrandr` \
		 `pkg-config --cflags gl`
PKG_LIBS= `pkg-config --libs xext` \
		 `pkg-config --libs x11` \
		 `pkg-config --libs xrandr` \
		 `pkg-config --libs gl`

CC=clang
CFLAGS=-Wall -Wextra -O2 -ffast-math -DCOOMER_VERSION="\"$(COOMER_VERSION)\"" $(PKG_INCS)
LIBS=  -lm $(PKG_LIBS)
LDFLAGS=

HEADERS=$(shell find ./src/ -type f -iname '*.h')
SRCS=$(shell find ./src/ -type f -iname '*.c')
BUILD_DIR=build/
OBJSRCS=$(shell basename -a $(shell realpath $(SRCS)))
OBJ=$(join $(addsuffix $(BUILD_DIR), $(dir $(OBJSRCS))), $(notdir $(OBJSRCS:.c=.o)))


all: options coomer

options:
	@echo coomer build options:
	@echo "CFLAGS  = $(CFLAGS)"
	@echo "LIBS    = $(LIBS)"
	@echo "LDFLAGS = $(LDFLAGS)"
	@echo "CC      = $(CC)"
	@echo "OBJ     = $(OBJ)"
	@echo "SRCS    = $(SRCS)"
	@echo "HEADERS = $(HEADERS)"

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

./$(BUILD_DIR)%.o:./src/%.c
	$(CC) $(CFLAGS) -o $@ -c $< 

coomer: $(HEADERS) $(SRCS) $(BUILD_DIR) $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LIBS) $(LDFLAGS)

sanitize:
	$(CFLAGS) += -fsanitize=address -fno-omit-frame-pointer -g
	$(LIBS) += -lasan
	LD_PRELOAD=/usr/lib/libasan.so make all

clean:
	rm -f coomer $(OBJ) 

.PHONY: all options clean
