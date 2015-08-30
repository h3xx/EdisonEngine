#pragma once

#include "core/boundingbox.h"
#include "core/mesh.h"
#include "object.h"
#include "portal.h"

#include <cstdint>
#include <memory>
#include <vector>

class btRigidBody;

namespace engine
{
struct EngineContainer;
}

namespace loader
{
class Level;
}

namespace world
{

namespace core
{
struct SpriteBuffer;
struct BaseMesh;
struct Frustum;
} // namespace core

struct StaticMesh;
struct RoomSprite;
struct Entity;
struct World;

enum class PenetrationConfig;
enum class DiagonalType;

struct RoomSector
{
    uint32_t                    trig_index; // Trigger function index.
    int32_t                     box_index;

    uint32_t                    flags;      // Climbability, death etc.
    uint32_t                    material;   // Footstep sound and footsteps.

    int32_t                     floor;
    int32_t                     ceiling;

    RoomSector        *sector_below;
    RoomSector        *sector_above;
    std::shared_ptr<Room> owner_room;    // Room that contain this sector

    int16_t                     index_x;
    int16_t                     index_y;
    btVector3 position;

    btVector3                   ceiling_corners[4];
    DiagonalType                ceiling_diagonal_type;
    PenetrationConfig           ceiling_penetration_config;

    btVector3                   floor_corners[4];
    DiagonalType                floor_diagonal_type;
    PenetrationConfig           floor_penetration_config;

    int32_t                     portal_to_room;

    RoomSector* getLowestSector();
    RoomSector* getHighestSector();

    RoomSector* checkFlip();
    RoomSector* checkBaseRoom();
    RoomSector* checkAlternateRoom();
    RoomSector* checkPortalPointerRaw();
    RoomSector* checkPortalPointer();
    bool is2SidePortals(RoomSector* s2);
    bool similarCeiling(RoomSector* s2, bool ignore_doors);
    bool similarFloor(RoomSector* s2, bool ignore_doors);
    btVector3 getFloorPoint();
    btVector3 getCeilingPoint();
};

struct Room : public Object
{
    uint32_t                    id;                                             // room's ID
    uint32_t                    flags;                                          // room's type + water, wind info
    int16_t                     light_mode;                                     // (present only in TR2: 0 is normal, 1 is flickering(?), 2 and 3 are uncertain)
    uint8_t                     reverb_info;                                    // room reverb type
    uint8_t                     water_scheme;
    uint8_t                     alternate_group;

    bool active;                                         // flag: is active
    bool hide;                                           // do not render
    std::shared_ptr<core::BaseMesh> mesh;                                           // room's base mesh
    //struct bsp_node_s          *bsp_root;                                       // transparency polygons tree; next: add bsp_tree class as a bsp_tree header
    core::SpriteBuffer *sprite_buffer;               // Render data for sprites

    std::vector<std::shared_ptr<StaticMesh>> static_mesh;
    std::vector<RoomSprite> sprites;

    std::vector<std::shared_ptr<engine::EngineContainer>> containers;                                     // engine containers with moveables objects

    core::BoundingBox boundingBox;
    btTransform transform;                                  // GL transformation matrix
    btScalar                    ambient_lighting[3];

    std::vector<core::Light> lights;

    std::vector<Portal> portals;                                        // room portals array
    std::shared_ptr<Room> alternate_room;                                 // alternative room pointer
    std::shared_ptr<Room> base_room;                                      // base room == room->alternate_room->base_room

    uint16_t                    sectors_x;
    uint16_t                    sectors_y;
    std::vector<RoomSector> sectors;

    std::vector<std::unique_ptr<core::Frustum>> frustum;
    uint16_t                    max_path;                                       // maximum number of portals from camera to this room

    std::vector<std::shared_ptr<Room>> near_room_list;
    std::vector<std::shared_ptr<Room>> overlapped_room_list;
    std::unique_ptr<btRigidBody> bt_body;

    std::unique_ptr<engine::EngineContainer> self;

    void enable();
    void disable();
    void swapToAlternate();
    void swapToBase();
    Room *checkFlip();
    void swapPortals(std::shared_ptr<Room> dest_room); //Swap room portals of input room to destination room
    void swapItems(std::shared_ptr<Room> dest_room);   //Swap room items of input room to destination room
    void buildNearRoomsList();
    void buildOverlappedRoomsList();

    bool isJoined(Room *r2);
    bool isOverlapped(Room *r1);
    bool isInNearRoomsList(const Room &r) const;
    bool hasSector(int x, int y);//If this room contains a sector
    void empty();
    void addEntity(Entity *entity);
    bool removeEntity(Entity* entity);
    void addToNearRoomsList(std::shared_ptr<Room> r);

    bool isPointIn(const btVector3& dot)
    {
        return boundingBox.contains(dot);
    }

    RoomSector* getSectorRaw(const btVector3 &pos);
    RoomSector* getSectorXYZ(const btVector3 &pos);

    void genMesh(World *world, uint32_t room_index, const std::unique_ptr<loader::Level>& tr);
};

} // namespace world
