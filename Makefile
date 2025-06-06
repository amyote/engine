CC=gcc
SC=glslc
CFLAGS=-Wall -Wvla -O2 -march=native $(shell sdl2-config --cflags) -g
LDFLAGS=$(shell sdl2-config --libs) -lSDL2_image -lvulkan -lm

SRC=src
OBJ=obj
RES=res
SHD=shaders
HDRS=$(wildcard $(SRC)/*.h)
SRCS=$(wildcard $(SRC)/*.c)
OBJS=$(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(SRCS))
SHDS=$(wildcard $(SHD)/shader.*)
SPVS=$(patsubst $(SHD)/shader.%, $(RES)/%.spv, $(SHDS))
BIN=Vk

BIN: $(OBJS) $(SPVS)
	$(CC) $(filter %.o, $^) -o $(BIN) $(LDFLAGS)

$(OBJ)/%.o: $(SRC)/%.c $(HDRS) objectdirectory
	$(CC) $(CFLAGS) -c $< -o $@

$(RES)/%.spv: $(SHD)/shader.% resourcedirectory
	$(SC) $< -o $@

objectdirectory:
	mkdir -p obj

resourcedirectory:
	mkdir -p res

.PHONY: clean

clean:
	rm $(OBJS)
	rm $(BIN)
	rm $(SPVS)

