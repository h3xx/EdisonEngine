#include "frustum.h"

#include <cstdio>
#include <cstdlib>

#include <LinearMath/btScalar.h>

#include "engine/engine.h"
#include "orientedboundingbox.h"
#include "polygon.h"
#include "render/render.h"
#include "util/vmath.h"
#include "world/camera.h"
#include "world/portal.h"
#include "world/room.h"
#include "world/world.h"

namespace world
{
namespace core
{

void Frustum::prepareSplit(const Portal& p)
{
    vertices = p.vertices;
    norm = p.normal;
    norm.mirrorNormal();
    parent = nullptr;
}

bool Frustum::splitByPlane(const util::Plane& splitPlane)
{
    assert(!vertices.empty());

    std::vector<btVector3> buf;

    btVector3* nextVertex = &vertices.front();
    btVector3* currentVertex = &vertices.back();

    btScalar dist[2];
    dist[0] = splitPlane.distance(*currentVertex);

    // run through all adjacent vertices
    for(size_t i = 0; i < vertices.size(); i++)
    {
        dist[1] = splitPlane.distance(*nextVertex);

        if(dist[1] > SplitEpsilon)
        {
            if(dist[0] < -SplitEpsilon)
            {
                buf.emplace_back(splitPlane.rayIntersect(*currentVertex, *nextVertex - *currentVertex));            // Shifting...
            }
            buf.emplace_back(*nextVertex);   // Adding...
        }
        else if(dist[1] < -SplitEpsilon)
        {
            if(dist[0] > SplitEpsilon)
            {
                buf.emplace_back(splitPlane.rayIntersect(*currentVertex, *nextVertex - *currentVertex));
            }
        }
        else
        {
            buf.emplace_back(*nextVertex);   // Adding...
        }

        currentVertex = nextVertex;
        ++nextVertex;
        dist[0] = dist[1];
    }

    if(buf.size() <= 2)       // Nothing was added or degenerative.
    {
        vertices.clear();
        return false;
    }

#if 0
    p->vertex_count = added;
    memcpy(p->vertex, buf, added * 3 * sizeof(btScalar));
#else
    // filter repeating (too closest) points
    nextVertex = &buf.front();
    currentVertex = &buf.back();
    vertices.clear();
    for(size_t i = 0; i<buf.size(); i++)
    {
        if(currentVertex->distance2(*nextVertex) > SplitEpsilon * SplitEpsilon)
        {
            vertices.emplace_back(*nextVertex);
        }
        currentVertex = nextVertex;
        ++nextVertex;
    }

    if(vertices.size() <= 2)
    {
        vertices.clear();
        return false;
    }
#endif

    return true;
}

void Frustum::genClipPlanes(const Camera& cam)
{
    if(vertices.empty())
        return;

    planes.resize(vertices.size());

    const btVector3* curr_v = &vertices.back();
    const btVector3* prev_v = curr_v - 1;

    for(size_t i = 0; i < vertices.size(); i++)
    {
        const btVector3* next_v = &vertices[i];
        auto V1 = *prev_v - cam.getPosition();                    // POV-vertex vector
        auto V2 = *prev_v - *curr_v;                       // vector connecting neighbor vertices
        V1.normalize();
        V2.normalize();
        planes[i].assign(V1, V2, *curr_v);

        prev_v = curr_v;
        curr_v = next_v;
    }
}

/*
 * receiver - points to the base room frustum, which portal leads to - it's taken from the portal!
 * returns a pointer to newly generated frustum.
 */
Frustum* Frustum::portalFrustumIntersect(const Portal& portal, const Frustum& emitter, const Camera& cam)
{
    if(!portal.dest_room)
        return nullptr;

    if(portal.normal.distance(cam.getPosition()) < -SplitEpsilon)    // non face or degenerate to the line portal
    {
        return nullptr;
    }

    if(!portal.dest_room->frustum.empty() && emitter.hasParent(*portal.dest_room->frustum.front()))
    {
        return nullptr;                                                        // Abort infinite loop!
    }

    bool in_dist = false, in_face = false;
    for(const btVector3& v : portal.vertices)
    {
        if(!in_dist && cam.frustum.norm.distance(v) < cam.m_distFar)
            in_dist = true;
        if(!in_face && emitter.norm.distance(v) > 0.0)
            in_face = true;
        if(in_dist && in_face)
            break;
    }

    if(!in_dist || !in_face)
        return nullptr;

    /*
     * Search for the first free room's frustum
     */
    portal.dest_room->frustum.emplace_back(new Frustum());
    Frustum* current_gen = portal.dest_room->frustum.back().get();

    current_gen->prepareSplit(portal);                       // prepare for clipping

    if(current_gen->splitByPlane(emitter.norm))               // splitting by main frustum clip plane
    {
        for(size_t i = 0; i < emitter.vertices.size(); i++)
        {
            const auto& n = emitter.planes[i];
            if(!current_gen->splitByPlane(n))
            {
                portal.dest_room->frustum.pop_back();
                return nullptr;
            }
        }

        current_gen->genClipPlanes(cam);                      // all is OK, let's generate clip planes

        current_gen->parent = &emitter;                                      // add parent pointer
        current_gen->parents_count = emitter.parents_count + 1;
        if(portal.dest_room->max_path < current_gen->parents_count)
        {
            portal.dest_room->max_path = current_gen->parents_count;       // maximum path to the room
        }
        return current_gen;
    }

    portal.dest_room->frustum.pop_back();

    return nullptr;
}

/*
 ************************* END FRUSTUM MANAGER IMPLEMENTATION*******************
 */

 /**
  * This function breaks looped recursive frustums.
  * If room has a frustum which is a parent to current frustum, function returns
  * true, and we break the loop.
  */

bool Frustum::hasParent(const Frustum& parent) const
{
    const Frustum* frustum = this;
    while(frustum)
    {
        if(&parent == frustum)
            return true;
        frustum = frustum->parent;
    }
    return false;
}

/**
 * Check polygon visibility through the portal.
 * This method is not for realtime since check is generally more expensive than rendering ...
 */
bool Frustum::isPolyVisible(const Polygon& p, const Camera& cam) const
{
    if(!p.double_side && p.plane.distance(cam.getPosition()) < 0.0)
    {
        return false;
    }

    // Direction from the camera position to an arbitrary vertex frustum
    assert(!vertices.empty());
    auto dir = vertices[0] - cam.getPosition();
    btScalar lambda = 0;

    // Polygon fits whole frustum (shouldn't happen, but we check anyway)
    if(p.rayIntersect(dir, cam.getPosition(), &lambda))
    {
        return true;
    }

    // Generate queue order...
    const util::Plane* nextPlane = &planes.front();
    // 3 neighboring clipping planes
    const util::Plane* currentPlane = &planes.back();
    const util::Plane* prevPlane = currentPlane - 1;
    // in case no intersection
    bool ins = true;
    // iterate through all the planes of this frustum
    for(size_t i = 0; i<vertices.size(); i++)
    {
        // Queue vertices for testing
        const Vertex* prevVertex = &p.vertices.back();
        // signed distance from the current point to the previous plane
        btScalar dist0 = currentPlane->distance(prevVertex->position);
        bool outs = true;
        // iterate through all the vertices of the polygon
        for(const Vertex& currentVertex : p.vertices)
        {
            btScalar dist1 = currentPlane->distance(currentVertex.position);
            // the split point in the plane
            if(std::abs(dist0) < SplitEpsilon)
            {
                if((prevPlane->distance(prevVertex->position) > -SplitEpsilon) &&
                   (nextPlane->distance(prevVertex->position) > -SplitEpsilon) &&
                   (norm.distance(prevVertex->position) > -SplitEpsilon))
                {
                    // Frustum-vertex intersection test is passed.
                    return true;
                }
            }

            // vertices from different sides of the plane (or on it)
            if((dist0 * dist1 < 0) && std::abs(dist1) >= SplitEpsilon)
            {
                // vector connecting vertices
                dir = currentVertex.position - prevVertex->position;
                // We are looking for the point of intersection
                const btVector3 T = currentPlane->rayIntersect(prevVertex->position, dir);
                if((prevPlane->distance(T) > -SplitEpsilon) && (nextPlane->distance(T) > -SplitEpsilon))
                {
                    // Frustum-ray intersection test is passed.
                    return true;
                }
            }

            // point is outside
            if(dist1 < -SplitEpsilon)
            {
                ins = false;
            }
            else
            {
                outs = false;
            }

            // We moved all the vertices of the polygon
            prevVertex = &currentVertex;
            // We moved all distances
            dist0 = dist1;
            // finished with all polygon vertices
        }

        if(outs)
        {
            // all points are outside of the current plane - definetly exit.
            return false;
        }
        // We moved all clipping planes
        prevPlane = currentPlane;
        currentPlane = nextPlane;
        ++nextPlane;
        // finished with all planes of this frustum
    }
    if(ins)
    {
        // all the vertices are inside - test is passed.
        return true;
    }

    return false;
}

/**
 *
 * @param bbmin - aabb corner (x_min, y_min, z_min)
 * @param bbmax - aabb corner (x_max, y_max, z_max)
 * @return true if aabb is in frustum.
 */
bool Frustum::isAABBVisible(const btVector3& bbmin, const btVector3& bbmax, const Camera& cam) const
{
    Polygon poly;
    poly.vertices.resize(4);
    bool ins = true;

    /* X_AXIS */

    poly.plane.normal[1] = 0.0;
    poly.plane.normal[2] = 0.0;
    if(cam.getPosition()[0] < bbmin[0])
    {
        poly.plane.normal[0] = -1.0;
        poly.plane.dot = -bbmin[0];
        poly.vertices[0].position[0] = bbmin[0];
        poly.vertices[0].position[1] = bbmax[1];
        poly.vertices[0].position[2] = bbmax[2];

        poly.vertices[1].position[0] = bbmin[0];
        poly.vertices[1].position[1] = bbmin[1];
        poly.vertices[1].position[2] = bbmax[2];

        poly.vertices[2].position[0] = bbmin[0];
        poly.vertices[2].position[1] = bbmin[1];
        poly.vertices[2].position[2] = bbmin[2];

        poly.vertices[3].position[0] = bbmin[0];
        poly.vertices[3].position[1] = bbmax[1];
        poly.vertices[3].position[2] = bbmin[2];

        if(isPolyVisible(poly, cam))
        {
            return true;
        }
        ins = false;
    }
    else if(cam.getPosition()[0] > bbmax[0])
    {
        poly.plane.normal[0] = 1.0;
        poly.plane.dot = bbmax[0];
        poly.vertices[0].position[0] = bbmax[0];
        poly.vertices[0].position[1] = bbmax[1];
        poly.vertices[0].position[2] = bbmax[2];

        poly.vertices[1].position[0] = bbmax[0];
        poly.vertices[1].position[1] = bbmin[1];
        poly.vertices[1].position[2] = bbmax[2];

        poly.vertices[2].position[0] = bbmax[0];
        poly.vertices[2].position[1] = bbmin[1];
        poly.vertices[2].position[2] = bbmin[2];

        poly.vertices[3].position[0] = bbmax[0];
        poly.vertices[3].position[1] = bbmax[1];
        poly.vertices[3].position[2] = bbmin[2];

        if(isPolyVisible(poly, cam))
        {
            return true;
        }
        ins = false;
    }

    /* Y AXIS */

    poly.plane.normal[0] = 0.0;
    poly.plane.normal[2] = 0.0;
    if(cam.getPosition()[1] < bbmin[1])
    {
        poly.plane.normal[1] = -1.0;
        poly.plane.dot = -bbmin[1];
        poly.vertices[0].position[0] = bbmax[0];
        poly.vertices[0].position[1] = bbmin[1];
        poly.vertices[0].position[2] = bbmax[2];

        poly.vertices[1].position[0] = bbmin[0];
        poly.vertices[1].position[1] = bbmin[1];
        poly.vertices[1].position[2] = bbmax[2];

        poly.vertices[2].position[0] = bbmin[0];
        poly.vertices[2].position[1] = bbmin[1];
        poly.vertices[2].position[2] = bbmin[2];

        poly.vertices[3].position[0] = bbmax[0];
        poly.vertices[3].position[1] = bbmin[1];
        poly.vertices[3].position[2] = bbmin[2];

        if(isPolyVisible(poly, cam))
        {
            return true;
        }
        ins = false;
    }
    else if(cam.getPosition()[1] > bbmax[1])
    {
        poly.plane.normal[1] = 1.0;
        poly.plane.dot = bbmax[1];
        poly.vertices[0].position[0] = bbmax[0];
        poly.vertices[0].position[1] = bbmax[1];
        poly.vertices[0].position[2] = bbmax[2];

        poly.vertices[1].position[0] = bbmin[0];
        poly.vertices[1].position[1] = bbmax[1];
        poly.vertices[1].position[2] = bbmax[2];

        poly.vertices[2].position[0] = bbmin[0];
        poly.vertices[2].position[1] = bbmax[1];
        poly.vertices[2].position[2] = bbmin[2];

        poly.vertices[3].position[0] = bbmax[0];
        poly.vertices[3].position[1] = bbmax[1];
        poly.vertices[3].position[2] = bbmin[2];

        if(isPolyVisible(poly, cam))
        {
            return true;
        }
        ins = false;
    }

    /* Z AXIS */

    poly.plane.normal[0] = 0.0;
    poly.plane.normal[1] = 0.0;
    if(cam.getPosition()[2] < bbmin[2])
    {
        poly.plane.normal[2] = -1.0;
        poly.plane.dot = -bbmin[2];
        poly.vertices[0].position[0] = bbmax[0];
        poly.vertices[0].position[1] = bbmax[1];
        poly.vertices[0].position[2] = bbmin[2];

        poly.vertices[1].position[0] = bbmin[0];
        poly.vertices[1].position[1] = bbmax[1];
        poly.vertices[1].position[2] = bbmin[2];

        poly.vertices[2].position[0] = bbmin[0];
        poly.vertices[2].position[1] = bbmin[1];
        poly.vertices[2].position[2] = bbmin[2];

        poly.vertices[3].position[0] = bbmax[0];
        poly.vertices[3].position[1] = bbmin[1];
        poly.vertices[3].position[2] = bbmin[2];

        if(isPolyVisible(poly, cam))
        {
            return true;
        }
        ins = false;
    }
    else if(cam.getPosition()[2] > bbmax[2])
    {
        poly.plane.normal[2] = 1.0;
        poly.plane.dot = bbmax[2];
        poly.vertices[0].position[0] = bbmax[0];
        poly.vertices[0].position[1] = bbmax[1];
        poly.vertices[0].position[2] = bbmax[2];

        poly.vertices[1].position[0] = bbmin[0];
        poly.vertices[1].position[1] = bbmax[1];
        poly.vertices[1].position[2] = bbmax[2];

        poly.vertices[2].position[0] = bbmin[0];
        poly.vertices[2].position[1] = bbmin[1];
        poly.vertices[2].position[2] = bbmax[2];

        poly.vertices[3].position[0] = bbmax[0];
        poly.vertices[3].position[1] = bbmin[1];
        poly.vertices[3].position[2] = bbmax[2];

        if(isPolyVisible(poly, cam))
        {
            return true;
        }
        ins = false;
    }

    return ins;
}

bool Frustum::isOBBVisible(const OrientedBoundingBox& obb, const Camera& cam) const
{
    bool ins = true;
    for(const Polygon& p : obb.polygons)
    {
        auto t = p.plane.distance(cam.getPosition());
        if((t > 0.0) && isPolyVisible(p, cam))
        {
            return true;
        }
        if(ins && (t > 0))
        {
            ins = false;
        }
    }

    return ins;
}

} // namespace core
} // namespace world
