#include <map>
#include <vector>
#include <string>
#include <list>

#include "DetourCommon.h"
#include "DetourNavMesh.h"
#include "DetourNavMeshQuery.h"

typedef struct _DVECTOR3
{
    union {
        struct {
            float x;
            float y;
            float z;
        };
        float m[3];
    };
    _DVECTOR3( float _x, float _y, float _z):x(_x),y(_y),z(_z) {}
    _DVECTOR3() {}
} VECTOR, VECTOR3;



static const float POINT_EXT = 0.5f;
static const float BOX_EXT[] = {POINT_EXT, 1000.f, POINT_EXT};

class Navmesh
{
public:
    explicit Navmesh(const char* _nav_path);
    ~Navmesh();

    typedef std::vector<VECTOR3> Path;

    bool findpath(const VECTOR3* _start, const VECTOR3* _end, Path* _path_point, unsigned short exclude=0);
    bool raycast(const VECTOR3* _start, const VECTOR3* _end, VECTOR3* _hit_pos, float* _rate, unsigned short exclude=0);
    bool move_along_surface(const VECTOR3* _start, const VECTOR3* _end, VECTOR3* _target);

    bool get_height(const VECTOR3* _pos, float* _height);

    bool find_random_pos_over_map(VECTOR3* _rand_pt, unsigned short exclude=0);
    bool find_random_pos(const VECTOR3* _pos, float radius, VECTOR3* _rand_pt, unsigned short exclude=0);
    bool find_reasonal_pos(const VECTOR3* _pos, float radius, VECTOR3*_closest_pt, unsigned short exclude=0);
    bool print_tiles();

    unsigned int get_querier_count();

    int get_errno();

    bool is_good() const
    {
        return initialized_;
    }

    VECTOR3 border_max;
    VECTOR3 border_min;

    static const int ERR_UNKNOWN = -1;
    static const int ERR_FILE_CANNOT_OPEN = 101;
    static const int ERR_VERSION_NOT_MATCH = 102;
    static const int ERR_NAVMESH_CANNOT_INIT = 103;
    static const int ERR_NAVMESH_READ_TILE_FAIL = 104;

private:

    static const int MAX_POLYS = 128;
    struct NavQuerier
    {
        float straight_path[MAX_POLYS*3];
        dtPolyRef polys[MAX_POLYS];
        dtNavMeshQuery *query;
    };
    
    Navmesh::NavQuerier* nq;

    bool find_nearest_poly(const VECTOR3* _pos, dtPolyRef*_poly_ptr, VECTOR3*_closest_ptr, dtQueryFilter* _filter);
    bool find_nearest_poly_q(const VECTOR3* _pos, dtPolyRef*_poly_ptr, VECTOR3*_closest_ptr, dtQueryFilter* _filter, NavQuerier* _querier);

    const dtNavMesh* navmesh_;
    dtNavMesh* new_navmesh(const char* _path);

    int errcode;
    void set_errno(const int e);

    bool read_tile(FILE *fp, dtNavMesh *_mesh);

    struct NavMeshSetHeader
    {
#ifdef RECAST_NEW_VERSION
        int magic;
#endif
        int version;
        int numTiles;
        dtNavMeshParams params;
    };

    struct NavMeshTileHeader
    {
        dtTileRef tileRef;
        int dataSize;
    };

    static const int NAVMESHSET_VERSION = 1;

    NavQuerier* get_querier();
    NavQuerier* new_querier();
    void put_querier(NavQuerier* querier);

    bool make_border(const dtNavMesh* _mesh);

    bool initialized_;
};