#pragma once

#include "loader/datatypes.h"


namespace render
{
    struct PortalTracer
    {
        gameplay::BoundingBox boundingBox{-1, -1, 0, 1, 1, 0};
        const loader::Portal* lastPortal = nullptr;


        bool checkVisibility(const loader::Portal* portal, const gameplay::Camera& camera)
        {
            const auto portalToCam = glm::vec3{camera.getInverseViewMatrix()[3]} - portal->vertices[0].toRenderSystem();
            if( glm::dot(portal->normal.toRenderSystem(), portalToCam) < 0 )
            {
                return false; // wrong orientation (normals must face the camera)
            }

            int numBehind = 0, numTooFar = 0;
            glm::vec2 screen[4];

            gameplay::BoundingBox portalBB{0, 0, 0, 0, 0, 0};
            portalBB.min = {1,1,0};
            portalBB.max = {-1,-1,0};

            for( int i = 0; i < 4; ++i )
            {
                screen[i] = projectOnScreen(portal->vertices[i].toRenderSystem(), camera, numBehind, numTooFar);
                portalBB.min.x = std::min(portalBB.min.x, screen[i].x);
                portalBB.min.y = std::min(portalBB.min.y, screen[i].y);
                portalBB.max.x = std::max(portalBB.max.x, screen[i].x);
                portalBB.max.y = std::max(portalBB.max.y, screen[i].y);

                // the first vertex must set the boundingbox to a valid state
                BOOST_ASSERT(portalBB.min.x <= portalBB.max.x);
                BOOST_ASSERT(portalBB.min.y <= portalBB.max.y);
            }

            if( numBehind == 4 || numTooFar == 4 )
                return false;

            BOOST_ASSERT(numBehind <= 3);

            // If (for some reason) our BBox is only a straight line, expand it to full size perpendicular to that line
            if(util::fuzzyEqual(portalBB.min.x, portalBB.max.x, std::numeric_limits<float>::epsilon()))
            {
                portalBB.min.x = -1;
                portalBB.max.x = 1;
            }
            if(util::fuzzyEqual(portalBB.min.y, portalBB.max.y, std::numeric_limits<float>::epsilon()))
            {
                portalBB.min.y = -1;
                portalBB.max.y = 1;
            }

#if 0
            // now check for denormalized boxes
            if(std::abs(portalBB.max.x - portalBB.min.x) / std::abs(portalBB.max.y - portalBB.min.y) > 1000)
            {
                // horizontal box, expand y
                portalBB.min.y = -1;
                portalBB.max.y = 1;
            }
            if(std::abs(portalBB.max.y - portalBB.min.y) / std::abs(portalBB.max.x - portalBB.min.x) > 1000)
            {
                // vertical box, expand x
                portalBB.min.x = -1;
                portalBB.max.x = 1;
            }
#endif

            boundingBox.min.x = std::max(portalBB.min.x, boundingBox.min.x);
            boundingBox.min.y = std::max(portalBB.min.y, boundingBox.min.y);
            boundingBox.max.x = std::min(portalBB.max.x, boundingBox.max.x);
            boundingBox.max.y = std::min(portalBB.max.y, boundingBox.max.y);

            lastPortal = portal;

            return boundingBox.min.x < boundingBox.max.x && boundingBox.min.y < boundingBox.max.y;
        }


        uint16_t getLastDestinationRoom() const
        {
            return getLastPortal()->adjoining_room;
        }


        const loader::Portal* getLastPortal() const
        {
            Expects(lastPortal != nullptr);
            return lastPortal;
        }


    private:
        static glm::vec2 projectOnScreen(glm::vec3 vertex,
                                         const gameplay::Camera& camera,
                                         int& numBehind,
                                         int& numTooFar)
        {
            static const auto sgn = [](float x) -> float { return x < 0 ? -1 : 1; };

            vertex = glm::vec3(camera.getViewMatrix() * glm::vec4(vertex, 1));

            if( -vertex.z <= 0 )
            {
                ++numBehind;
                glm::vec2 normalizedScreen;
                normalizedScreen.x = sgn(vertex.x);
                normalizedScreen.y = sgn(vertex.y);
                return normalizedScreen;
            }

            if( -vertex.z > camera.getFarPlane() )
                ++numTooFar;

            glm::vec4 tmp{vertex, 1};
            tmp = camera.getProjectionMatrix() * tmp;

            glm::vec2 screen{glm::clamp(tmp.x / tmp.w, -1.0f, 1.0f), glm::clamp(tmp.y / tmp.w, -1.0f, 1.0f)};
            return screen;
        }
    };
}
