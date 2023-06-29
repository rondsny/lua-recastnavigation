extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <map>

#include "navmesh.h"

#define EXCLUDE_MAX 0xffff

static inline Navmesh* _check_navmesh(lua_State *L){
    Navmesh **ud = (Navmesh **)lua_touserdata(L, 1);
    if(ud==NULL){
        luaL_error(L, "no navmesh found");
    }
    return *ud;
}

// 初始化闭包
static int _new(lua_State *L){
    const char *nav_path = luaL_checkstring(L, 1);
    Navmesh *ud = new Navmesh(nav_path);
    Navmesh **sl = (Navmesh**) lua_newuserdata(L, sizeof(Navmesh*));
    *sl = ud;
    lua_pushvalue(L, lua_upvalueindex(1));
    lua_setmetatable(L, -2);
    return 1;
}


static int _release(lua_State *L){
    Navmesh* navmesh = _check_navmesh(L);
    delete navmesh;
    return 0;
}

static int l_test_data(lua_State *L){
    Navmesh *navmesh = _check_navmesh(L);
    printf("test addr %p\n", navmesh);
    lua_pushstring(L, "abc");
    return 1;
}

static int l_get_border(lua_State *L){
    Navmesh *navmesh = _check_navmesh(L);
    lua_pushnumber(L, navmesh->border_max.x);
    lua_pushnumber(L, navmesh->border_max.y);
    lua_pushnumber(L, navmesh->border_max.z);
    lua_pushnumber(L, navmesh->border_min.x);
    lua_pushnumber(L, navmesh->border_min.y);
    lua_pushnumber(L, navmesh->border_min.z);
    return 6;
}

static int l_get_triangles(lua_State *L){
    Navmesh *navmesh = _check_navmesh(L);
    bool ret = navmesh->print_tiles();
    if(ret){
        printf("findpath success\n");
        return 0;
    }else{
        printf("findpath fail\n");
        return 0;
    }
}

static int l_findpath(lua_State *L){
    Navmesh *navmesh = _check_navmesh(L);
    float sx = luaL_checknumber(L, 2);
    float sy = luaL_checknumber(L, 3);
    float sz = luaL_checknumber(L, 4);
    float ex = luaL_checknumber(L, 5);
    float ey = luaL_checknumber(L, 6);
    float ez = luaL_checknumber(L, 7);
    unsigned int exclude = luaL_checkinteger(L, 8);
    if(exclude==NULL || exclude>EXCLUDE_MAX){
        luaL_error(L, "exclude must be a number between 0 and 65535");
    }
    VECTOR3 start(sx, sy, sz);
    VECTOR3 end(ex, ey, ez);
    std::vector<VECTOR3> pathPoint;
    bool ret = navmesh->findpath(&start, &end, &pathPoint, exclude);
    if(ret){
        printf("findpath success\n");
        lua_newtable(L);
        int i = 1;
        
        std::vector<VECTOR3>::reverse_iterator rit = pathPoint.rbegin();
        for (; rit!= pathPoint.rend(); ++rit)
        {
            lua_pushnumber(L, i++);
            lua_newtable(L);
            lua_pushnumber(L, 1);
            lua_pushnumber(L, rit->x);
            lua_settable(L, -3);
            lua_pushnumber(L, 2);
            lua_pushnumber(L, rit->y);
            lua_settable(L, -3);
            lua_pushnumber(L, 3);
            lua_pushnumber(L, rit->z);
            lua_settable(L, -3);
            lua_settable(L, -3);
        }
        return 1;
    }else{
        printf("findpath fail\n");
        return 0;
    }
}


static int l_is_walkable(lua_State *L)
{
    Navmesh *navmesh = _check_navmesh(L);
    float ex = luaL_checknumber(L, 2);
    float ey = luaL_checknumber(L, 3);
    float ez = luaL_checknumber(L, 4);
    unsigned int exclude = luaL_checkinteger(L, 5);
    if(NULL==exclude || exclude>EXCLUDE_MAX){
        luaL_error(L, "exclude must be a number between 0 and 65535");
    }

    VECTOR3 vec(ex, ey, ez);
    VECTOR3 vec2;
    if (navmesh -> find_reasonal_pos(&vec, 0.02f, &vec2, exclude)){
        lua_pushboolean(L, true);
    }else{
        lua_pushboolean(L, false);
    }
    return 1;
}

static int l_reasonal_pos(lua_State *L)
{
    Navmesh *navmesh = _check_navmesh(L);
    double radius = luaL_checknumber(L, 2);
    float ex = luaL_checknumber(L, 3);
    float ey = luaL_checknumber(L, 4);
    float ez = luaL_checknumber(L, 5);
    unsigned int exclude = luaL_checkinteger(L, 6);
    if(NULL==exclude || exclude>EXCLUDE_MAX){
        luaL_error(L, "exclude must be a number between 0 and 65535");
    }

    VECTOR3 vec(ex, ey, ez);
    VECTOR3 vec2;
    if (navmesh -> find_reasonal_pos(&vec, 0.02f, &vec2, exclude)){
        lua_pushboolean(L, true);
        lua_pushnumber(L, vec2.x);
        lua_pushnumber(L, vec2.y);
        lua_pushnumber(L, vec2.z);
        return 4;
    }else{
        lua_pushboolean(L, false);
        return 1;
    }
}


static int l_raycast(lua_State *L)
{
    Navmesh *navmesh = _check_navmesh(L);
    float sx = luaL_checknumber(L, 2);
    float sy = luaL_checknumber(L, 3);
    float sz = luaL_checknumber(L, 4);
    float ex = luaL_checknumber(L, 5);
    float ey = luaL_checknumber(L, 6);
    float ez = luaL_checknumber(L, 7);
    unsigned int exclude = luaL_checkinteger(L, 8);
    
    VECTOR3 start(sx, sy, sz);
    VECTOR3 end(ex, ey, ez);

    VECTOR3 hit_vec;
    float rate;
    if (!navmesh->raycast(&start, &end, &hit_vec, &rate, exclude)){
        lua_pushboolean(L, false);
        return 1;
    }

    if (rate < 1.0){
        lua_pushnumber(L, rate);
        lua_pushnumber(L, hit_vec.x);
        lua_pushnumber(L, hit_vec.y);
        lua_pushnumber(L, hit_vec.z);
    }else{
        lua_pushnumber(L, rate);
        lua_pushnumber(L, hit_vec.x);
        lua_pushnumber(L, hit_vec.y);
        lua_pushnumber(L, hit_vec.z);
    }
    return 4;
}

static int l_random_pos(lua_State *L)
{
    Navmesh *navmesh = _check_navmesh(L);
    double radius = luaL_checknumber(L, 2);
    float ex = luaL_checknumber(L, 3);
    float ey = luaL_checknumber(L, 4);
    float ez = luaL_checknumber(L, 5);
    unsigned int exclude = luaL_checkinteger(L, 6);
    if(NULL==exclude || exclude>EXCLUDE_MAX){
        luaL_error(L, "exclude must be a number between 0 and 65535");
    }
    VECTOR3 vec(ex, ey, ez);

    VECTOR3 vec2;
    if (navmesh -> find_random_pos(&vec, radius, &vec2, exclude))
    {
        lua_pushnumber(L, vec2.x);
        lua_pushnumber(L, vec2.y);
        lua_pushnumber(L, vec2.z);
        return 3;
    }
    else
    {
        return 0;
    }
}

static int l_random_pos_over_map(lua_State *L)
{
    Navmesh *navmesh = _check_navmesh(L);
    unsigned int exclude = luaL_checkinteger(L, 2);
    if(NULL==exclude || exclude>EXCLUDE_MAX){
        luaL_error(L, "exclude must be a number between 0 and 65535");
    }

    VECTOR3 vec;
    if (navmesh -> find_random_pos_over_map(&vec, exclude))
    {
        lua_pushnumber(L, vec.x);
        lua_pushnumber(L, vec.y);
        lua_pushnumber(L, vec.z);
        return 3;
    }
    else
    {
        return 0;
    }
}

extern "C" int luaopen_navmeh(lua_State* L)
{
    luaL_checkversion(L);

    luaL_Reg l[] = {
        {"test_data", l_test_data},
        {"get_border", l_get_border},
        {"findpath", l_findpath},
        {"is_walkable", l_is_walkable},
        {"reasonal_pos", l_reasonal_pos},
        {"raycast", l_raycast},
        {"random_pos", l_random_pos},
        {"random_pos_over_map", l_random_pos_over_map},
        
        {"get_triangles", l_get_triangles},
        {NULL, NULL}
    };

    lua_createtable(L, 0, 2);

    luaL_newlib(L, l);
    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, _release);
    lua_setfield(L, -2, "__gc");

    lua_pushcclosure(L, _new, 1);

    return 1;
}
