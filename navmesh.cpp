
#include <cmath>
#include <cstdio>
#include <cfloat>
#include <cstring>
#include <map>
#include <algorithm>

#include "navmesh.h"

Navmesh::Navmesh( const char* _nav_path ) : initialized_(false)
{
    navmesh_ = new_navmesh(_nav_path);
    if(NULL == navmesh_) return ;

    if(!make_border(navmesh_)) return ;
    
    nq = new_querier();

    initialized_ = true;
}

Navmesh::~Navmesh()
{

}


bool Navmesh::make_border(const dtNavMesh* _mesh)
{
    int i,j;
    float m,n;
    float bmax[3], bmin[3];

    for(i = 0; i < _mesh->getMaxTiles(); i++)
    {
        const dtMeshTile* tile = _mesh->getTile(i);
        if(NULL == tile)
        {
            return false;
        }
        else
        {
            for(j=0; j<3; j++)
            {
                m = tile->header->bmax[j];
                n = tile->header->bmin[j];
                if(0 == i)
                {
                    bmax[j] = m;
                    bmin[j] = n;
                }
                else
                {
                    bmax[j] = m > bmax[j] ? m : bmax[j];
                    bmin[j] = n < bmin[j] ? n : bmin[j];
                }
            }
        }
    }

    border_max.x = bmax[0];
    border_max.y = bmax[1];
    border_max.z = bmax[2];

    border_min.x = bmin[0];
    border_min.y = bmin[1];
    border_min.z = bmin[2];
    return true;
}

int Navmesh::get_errno()
{
    return errcode;
}


void Navmesh::set_errno(const int err)
{
    errcode = err;
}

// 注意: 获取querier, 使用完后必须用put_querier把它放回池子, 否则会死锁!
Navmesh::NavQuerier* Navmesh::get_querier()
{
    return nq;
}

void Navmesh::put_querier(NavQuerier* querier)
{
}

Navmesh::NavQuerier* Navmesh::new_querier()
{
    dtNavMeshQuery *q = dtAllocNavMeshQuery();
    if (NULL == q)
    {
        return NULL;
    }
    else
    {

        if (DT_SUCCESS == q -> init(navmesh_, MAX_POLYS))
        {
            NavQuerier* querier = new NavQuerier();
            if (NULL == querier)
            {
                return NULL;
            }
            else
            {
                querier->query = q;
                return querier;
            }
        }
        else
        {
            return NULL;
        }
    }
}

dtNavMesh* Navmesh::new_navmesh(const char* _path )
{
    FILE* fp = fopen(_path, "rb");

    if(!fp)
    {
        set_errno(ERR_FILE_CANNOT_OPEN);
        return NULL;
    }

    NavMeshSetHeader header;
    fread(&header, sizeof(NavMeshSetHeader), 1, fp);

    if (header.version != NAVMESHSET_VERSION)
    {
        fclose(fp);
        set_errno(ERR_VERSION_NOT_MATCH);
        return NULL;
    }

    dtNavMesh* mesh = dtAllocNavMesh();
    if (!mesh || mesh->init(&header.params) != DT_SUCCESS)
    {
        fclose(fp);
        set_errno(ERR_NAVMESH_CANNOT_INIT);
        return NULL;
    }

    for (int i = 0; i < header.numTiles; ++i)
    {
        if(!read_tile(fp, mesh))
        {
            fclose(fp);
            set_errno(ERR_NAVMESH_READ_TILE_FAIL);
            return NULL;
        }
    }
    fclose(fp);
    return mesh;
}



bool Navmesh::findpath(const VECTOR3* _start, const VECTOR3* _end,
         Path* _path_point, unsigned short exclude)
{
    VECTOR3 start_pos, end_pos;
    dtPolyRef start_poly = 0, end_poly = 0;
    dtQueryFilter filter = dtQueryFilter();
    filter.setExcludeFlags(exclude);

    NavQuerier* querier = get_querier();
    if (NULL == querier)
    {
        return false;
    }

    if (!find_nearest_poly_q(_start, &start_poly, &start_pos, &filter, querier) ||
        !find_nearest_poly_q(_end, &end_poly, &end_pos, &filter, querier))
    {
        put_querier(querier);
        return false;
    }

    VECTOR3 hit_normal;
    dtStatus ret;
    int npolys = 0;
    float t = 0.0;

    // 先做raycast
    ret = querier->query->raycast(start_poly, &start_pos.x, &end_pos.x, &filter, &t,
        &hit_normal.x, querier->polys, &npolys, MAX_POLYS);

    if (DT_SUCCESS != ret)
    {
        put_querier(querier);
        return false;
    }

    if (t >= 1.0f) // raycast直达
    {
        _path_point->push_back(VECTOR3(start_pos));
        _path_point->push_back(VECTOR3(end_pos));
        put_querier(querier);
        return true;
    }

    // raycast无法直达
    ret = querier->query->findPath(start_poly, end_poly, &start_pos.x, &end_pos.x,
                &filter, querier->polys, &npolys, MAX_POLYS);

    if (DT_SUCCESS != ret || 0 == npolys)
    {
        put_querier(querier);
        return false;
    }

    int path_len = 0;
    ret = querier->query->findStraightPath(&start_pos.x, &end_pos.x, querier->polys,
            npolys, querier->straight_path, NULL, NULL, &path_len, MAX_POLYS);

    if (DT_SUCCESS != ret || path_len < 2)
    {
        put_querier(querier);
        return false;
    }

    _path_point->reserve(path_len);
    for (int i = 0; i < path_len; ++i)
    {
        _path_point->push_back(VECTOR3(querier->straight_path[i*3],
            querier->straight_path[i*3+1], querier->straight_path[i*3+2]));
    }

    put_querier(querier);
    return true;
}


bool Navmesh::move_along_surface(const VECTOR3* _start, const VECTOR3* _end, VECTOR3* _target)
{
    VECTOR3 start_pos, end_pos;
    dtPolyRef start_poly = 0;
    dtQueryFilter filter = dtQueryFilter();

    NavQuerier* querier = get_querier();
    if (NULL == querier)
    {
        return false;
    }

    if (!find_nearest_poly_q(_start, &start_poly, &start_pos, &filter, querier))
    {
        put_querier(querier);
        return false;
    }

    int visitedCount;
    dtStatus ret;
    ret = querier->query->moveAlongSurface(start_poly, &start_pos.x, &end_pos.x,
                &filter, &(_target->x), querier->polys, &visitedCount, MAX_POLYS);

    if (DT_SUCCESS != ret || 0 == visitedCount)
    {
        put_querier(querier);
        return false;
    }

    put_querier(querier);
    return true;
}

bool Navmesh::raycast( const VECTOR3* _start, const VECTOR3* _end,
                VECTOR3* _hit_pos, float* _rate, unsigned short exclude)
{
    dtPolyRef start_poly = 0;
    VECTOR3 start_pos;
    VECTOR3 end_pos(*_end);

    *_hit_pos = *_start;
    *_rate = 0.f;

    dtQueryFilter filter = dtQueryFilter();
    filter.setExcludeFlags(exclude);
    NavQuerier* querier = get_querier();
    if (NULL == querier)
    {
        return false;
    }

    if (!find_nearest_poly_q(_start, &start_poly, &start_pos, &filter, querier))
    {
        put_querier(querier);
        return false;
    }

    VECTOR3 hit_normal;
    float t = 0.0;


    end_pos.y = start_pos.y;
    int npolys = 0;
    dtStatus ret;
    ret = querier->query->raycast(start_poly, &start_pos.x, &end_pos.x, &filter, &t,
        &hit_normal.x, querier->polys, &npolys, MAX_POLYS);
    put_querier(querier);

    if (DT_SUCCESS != ret)
    {
        return false;
    }

    if (t >= 1.0f)
    {
        _hit_pos->x = end_pos.x;
        _hit_pos->y = end_pos.y;
        _hit_pos->z = end_pos.z;
    }
    else
    {
        _hit_pos->x = start_pos.x + (end_pos.x - start_pos.x) * t;
        _hit_pos->y = start_pos.y + (end_pos.y - start_pos.y) * t;
        _hit_pos->z = start_pos.z + (end_pos.z - start_pos.z) * t;
    }

    t = t > 1.0f ? 1.0f : t;
    *_rate = t;

    return true;
}

bool Navmesh::read_tile( FILE *fp, dtNavMesh *_mesh )
{
    NavMeshTileHeader tileHeader;
    fread(&tileHeader, sizeof(tileHeader), 1, fp);
    if (!tileHeader.tileRef || !tileHeader.dataSize)
    {
        return false;
    }
    unsigned char* data = (unsigned char*)dtAlloc(tileHeader.dataSize, DT_ALLOC_PERM);
    if (!data)
    {
        return false;
    }
    memset(data, 0, static_cast<unsigned int>(tileHeader.dataSize));
    fread(data, static_cast<unsigned int>(tileHeader.dataSize), 1, fp);

    _mesh->addTile(data, tileHeader.dataSize, DT_TILE_FREE_DATA, tileHeader.tileRef, 0);

    return true;
}


bool Navmesh::find_nearest_poly(const VECTOR3* _pos, dtPolyRef*_poly_ptr,
                                 VECTOR3*_closest_ptr, dtQueryFilter* _filter)
{
    NavQuerier* querier = get_querier();
    if (NULL == querier)
    {
        return false;
    }

    bool ret;
    ret = find_nearest_poly_q(_pos, _poly_ptr, _closest_ptr, _filter, querier);
    put_querier(querier);

    return ret;
}


bool Navmesh::find_nearest_poly_q(const VECTOR3* _pos, dtPolyRef*_poly_ptr,
             VECTOR3*_closest_ptr, dtQueryFilter* _filter, NavQuerier* querier)
{
    dtStatus ret;
    ret = querier->query->findNearestPoly(&_pos->x, &BOX_EXT[0], _filter, _poly_ptr, &_closest_ptr->x);
    return (DT_SUCCESS == ret) &&  (0 != _poly_ptr);
}

bool Navmesh::find_reasonal_pos(const VECTOR3* _pos, float radius,
             VECTOR3* _closest_pt, unsigned short exclude)
{
    NavQuerier* querier = get_querier();
    if (NULL == querier)
    {
        return false;
    }

    dtPolyRef poly_ptr;
    dtQueryFilter filter = dtQueryFilter();
    filter.setExcludeFlags(exclude);

    float box_ext[3] = {radius, 1000.f, radius};

    dtStatus ret;
    ret = querier->query->findNearestPoly(&_pos->x, &box_ext[0], &filter, &poly_ptr, &_closest_pt->x);
    put_querier(querier);

    return (DT_SUCCESS == ret) && (0 != poly_ptr);
}

static float frand()
{
    return (float)rand()/(float)RAND_MAX;
}

bool Navmesh::find_random_pos(const VECTOR3* _pos, float radius,
         VECTOR3* _rand_pt, unsigned short exclude)
{
    dtPolyRef start_poly = 0;
    VECTOR3 start_pos;

    dtQueryFilter filter = dtQueryFilter();
    filter.setExcludeFlags(exclude);
    if (!find_nearest_poly(_pos, &start_poly, &start_pos, &filter))
    {
        return false;
    }

    NavQuerier* querier = get_querier();
    if (NULL == querier)
    {
        return false;
    }

    dtPolyRef poly_ptr;

    dtStatus ret;
    ret = querier->query->findRandomPointAroundCircle(start_poly, &_pos->x, radius, &filter, frand, &poly_ptr, &_rand_pt->x);
    put_querier(querier);

    return (DT_SUCCESS == ret) && (0 != poly_ptr);
}


bool Navmesh::find_random_pos_over_map(VECTOR3* _rand_pt, unsigned short exclude)
{
    dtQueryFilter filter = dtQueryFilter();
    filter.setExcludeFlags(exclude);
    dtPolyRef randomRef;
    NavQuerier* querier = get_querier();
    if (NULL == querier)
    {
        return false;
    }
    dtStatus ret;
    ret = querier->query->findRandomPoint(&filter, frand, &randomRef, &_rand_pt->x);
    put_querier(querier);

    return (DT_SUCCESS == ret) && (0 != randomRef);
}

bool Navmesh::get_height( const VECTOR3* _pos, float* _height)
{
    dtPolyRef polyRef;
    VECTOR3 retPos;
    dtQueryFilter filter = dtQueryFilter();

    if (find_nearest_poly(_pos, &polyRef, &retPos, &filter))
    {
        *_height = retPos.y;
        return true;
    }
    else
    {
        return false;
    }
}

bool Navmesh::print_tiles()
{
	FILE* fp = fopen("map_data.py", "w");
    fprintf(fp, "DATA=[");
    int i,j;
    int count = 0;
    printf("getMaxTiles = %d\n", navmesh_->getMaxTiles());
    for(i = 0; i < navmesh_->getMaxTiles(); i++)
    {
        const dtMeshTile* tile = navmesh_->getTile(i);

        if(NULL == tile)
        {
            printf("tile[%d] is NULL\n", i);
            return false;
        }
        else
        {
            printf("tile[%d] is not NULL, %ld\n", i, (long int)(tile->polys));
            if(tile->header){
                printf("tile[%d] 多边形数量 = %d 顶点数量 = %d\n", i, tile->header->polyCount, tile->header->vertCount);
                for(int k = 0; k < tile->header->polyCount; k++)
                {
                    count++;
                    dtPoly* poly = &tile->polys[k];
                    // printf("\t多边形 poly[%d], 顶点数量 = %d\n", k, poly->vertCount);
                    fprintf(fp, "[[");
                    for(int l = 0; l < poly->vertCount; l++)
                    {
                        int vertIndex = poly->verts[l];
                        float x = tile->verts[vertIndex*3];
                        float y = tile->verts[vertIndex*3+1];
                        float z = tile->verts[vertIndex*3+2];
                        // printf("\t\t 顶点信息 pverts[%d] index = %d, 坐标是 (%f, %f, %f)\n", l, poly->verts[l], 
                        //     tile->verts[vertIndex*3], tile->verts[vertIndex*3+1],tile->verts[vertIndex*3+2]
                        //     );
                        fprintf(fp, "[%f, %f],", x, z);
                    }
                    fprintf(fp, "], %d],\n", k%10);
                }
            }
        }
    }
    fprintf(fp, "]\n");
    fprintf(fp, "BORDER_X_MIN=%f\n", border_min.x);
    fprintf(fp, "BORDER_X_MAX=%f\n", border_max.x);
    fprintf(fp, "BORDER_Z_MIN=%f\n", border_min.z);
    fprintf(fp, "BORDER_Z_MAX=%f\n", border_max.z);
	fclose(fp);
    return true;
}