
all: recast.so

CC = g++
CC1 = gcc
CFLAGS = -Wall -O0 -fPIC -g3 --shared
LUA_INCLUDE_DIR = /usr/include/lua
RECAST_INCLUDE_DIR = 3rd/recastnavigation/Detour/Include
# DEFS = -DRECAST_NEW_VERSION

recast.so: recast.h recast.cpp 3rd/recastnavigation/Detour/Source/*.cpp lua-recast.cpp
	$(CC) $(CFLAGS) -I$(LUA_INCLUDE_DIR) -I$(RECAST_INCLUDE_DIR) $(DEFS) -o $@ $^

test:
	lua test.lua

clean:
	rm -f *.so