CC = gcc
SRC_DIR = src
TEST_DIR = tests
BUILD_DIR = tests/build

TEST_SOURCES = $(wildcard $(TEST_DIR)/*.c)

TEST_TARGETS = $(patsubst $(TEST_DIR)/%.c, $(BUILD_DIR)/%, $(TEST_SOURCES))

MODULES = $(SRC_DIR)/display.c

all: $(TEST_TARGETS)

$(BUILD_DIR)/%: $(TEST_DIR)/%.c $(MODULES)
	$(CC) $(CFLAGS) $^ -o $@