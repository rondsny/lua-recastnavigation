# recastnavigation lua api

Lua API for [https://github.com/recastnavigation/recastnavigation.git](https://github.com/recastnavigation/recastnavigation.git).

![sample1](/test/images/sample1.png)


## API
- `findpath` 
- `is_walkable`
- `raycast`
- `random_pos`

## build
```sh
make all
# or
lua test.lua
```

## test
```sh
make test
```

## tools

Convert a navmesh file to a picture.
```sh
sh tools/test_save_all_png.sh
```