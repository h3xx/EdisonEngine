#include "tallblock.h"

#include "level/level.h"

namespace engine
{
    namespace items
    {
        void TallBlock::onFrameChanged(FrameChangeType frameChangeType)
        {
            ItemNode::onFrameChanged( frameChangeType );
            auto room = getCurrentRoom();
            getLevel().findFloorSectorWithClampedPosition( getPosition().toInexact(), &room );
            setCurrentRoom( room );

            if( m_flags2_02_toggledOn || !m_flags2_04_ready )
                return;

            m_flags2_02_toggledOn = true;
            m_flags2_04_ready = false;
            loader::Room::patchHeightsForBlock( *this, -2 * loader::SectorSize );
            auto pos = getPosition();
            pos.X = std::floor( pos.X / loader::SectorSize ) * loader::SectorSize + loader::SectorSize / 2;
            pos.Z = std::floor( pos.Z / loader::SectorSize ) * loader::SectorSize + loader::SectorSize / 2;
            setPosition( pos );
        }
    }
}