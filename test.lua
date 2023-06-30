local args = {...}
local path = "test/nav_data/sample1.navmesh"
if #args > 0 then
    path = args[1]
end

local Recast = require 'recast'

print("type 1>>>", type(Recast))
local obj = Recast(path)
print("type 2>>>", type(obj))

print("test_data", obj:test_data())

print("border", obj:get_border())

print("findpath 1", obj:findpath( 
    250, 19, 50, 
    325, 19, 100, 1024))

print("is_walkable test1", obj:is_walkable(250, 19, 50, 1024))
print("is_walkable test2", obj:is_walkable(200, 19, 50, 1024))

print("reasonal_pos", obj:reasonal_pos(2, 250, 19, 50, 1024))

print("raycast test1", obj:raycast(250, 19, 50, 325, 19, 100, 1024))
print("raycast test2", obj:raycast(200, 19, 50, 325, 19, 100, 1024))

print("random_pos test1", obj:random_pos(20, 250, 19, 50, 1024))
print("random_pos test2", obj:random_pos(20, 250, 19, 50, 1024))
print("random_pos test3", obj:random_pos(20, 250, 19, 50, 1024))

print("random_pos_over_map test1", obj:random_pos_over_map(1024))
print("random_pos_over_map test2", obj:random_pos_over_map(1024))
print("random_pos_over_map test3", obj:random_pos_over_map(1024))

print("get_triangles", obj:get_triangles("tools/map_data.py"))