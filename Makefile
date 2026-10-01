CC      ?= gcc
CSTD    ?= c17
USE_OMP ?= 1

SRC_DIR  := src
PROG_DIR := experiments
INC_DIR  := include
OBJ_DIR  := build/obj
BIN_DIR  := build/bin

CPPFLAGS := -I$(INC_DIR)
CFLAGS   := -Wall -Wextra -Wpedantic -O3 -std=$(CSTD)
LDFLAGS  :=
LDLIBS   := -lm

SRC   := $(wildcard $(SRC_DIR)/*.c)
OBJS  := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC))
DEPS  := $(OBJS:.o=.d)
PROGS := $(notdir $(basename $(wildcard $(PROG_DIR)/*.c)))

CC_VERSION     := $(shell $(CC) --version 2>/dev/null | head -n 1)
IS_ICX         := $(findstring icx,$(CC))
IS_APPLE_CLANG := $(findstring Apple clang,$(CC_VERSION))

# Link-time optimization
ifeq ($(IS_ICX),icx)
    CFLAGS  += -ipo
    LDFLAGS += -ipo
else
    CFLAGS  += -flto
    LDFLAGS += -flto
endif


# OpenMP
ifeq ($(USE_OMP),1)
    ifeq ($(IS_ICX),icx)
        CFLAGS  += -qopenmp
        LDFLAGS += -qopenmp
    else ifneq ($(IS_APPLE_CLANG),)
        # Apple clang needs libomp, e.g. from Homebrew
        OMP_PREFIX ?= $(shell brew --prefix libomp 2>/dev/null)
        CPPFLAGS   += -isystem $(OMP_PREFIX)/include
        CFLAGS     += -Xpreprocessor -fopenmp
        LDFLAGS    += -L$(OMP_PREFIX)/lib
        LDLIBS     += -lomp
    else
        CFLAGS  += -fopenmp
        LDFLAGS += -fopenmp
    endif
else
    CFLAGS += -Wno-unknown-pragmas
endif


.PHONY: all clean $(PROGS)

all: $(PROGS)

# One target per program: make escape, make basin, ...
$(PROGS): %: $(BIN_DIR)/%.x

$(OBJ_DIR) $(BIN_DIR):
	mkdir -p $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

$(BIN_DIR)/%.x: $(PROG_DIR)/%.c $(OBJS) | $(BIN_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(OBJS) $(LDFLAGS) $(LDLIBS) -o $@

clean:
	rm -rf build

-include $(DEPS)
