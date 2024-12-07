CC = gcc
CFLAGS = -Wall -Wextra -I../src
LDFLAGS = -lvmi -ljson-c

SRC_DIR = ../src
BUILD_DIR = .

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
TARGET = kaslr_spoofer

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(BUILD_DIR)/*.o $(TARGET)
