#pragma once

#include "dart.h"

namespace engine
{
namespace items
{
class DartGun final : public ModelItemNode
{
public:
    DartGun(const gsl::not_null<level::Level*>& level,
            const std::string& name,
            const gsl::not_null<const loader::Room*>& room,
            const loader::Item& item,
            const loader::SkeletalModelType& animatedModel)
            : ModelItemNode( level, name, room, item, true, animatedModel )
    {
    }

    void update() override
    {
        if( m_state.updateActivationTimeout() )
        {
            if( m_state.current_anim_state == 0 )
            {
                m_state.goal_anim_state = 1;
            }
        }
        else if( m_state.current_anim_state == 1 )
        {
            m_state.goal_anim_state = 0;
        }

        if( m_state.current_anim_state != 1 || m_state.frame_number != m_state.anim->firstFrame )
        {
            ModelItemNode::update();
            return;
        }

        auto axis = core::axisFromAngle( m_state.rotation.Y, 45_deg );
        BOOST_ASSERT( axis.is_initialized() );

        core::TRCoordinates d( 0, 512, 0 );

        switch( *axis )
        {
            case core::Axis::PosZ:
                d.Z += 412;
                break;
            case core::Axis::PosX:
                d.X += 412;
                break;
            case core::Axis::NegZ:
                d.Z -= 412;
                break;
            case core::Axis::NegX:
                d.X -= 412;
                break;
            default:
                break;
        }

        auto dart = getLevel()
                .createItem<Dart>( engine::TR1ItemId::Dart, m_state.position.room, m_state.rotation.Y,
                                   m_state.position.position - d, 0 );
        dart->activate();
        dart->m_state.triggerState = engine::items::TriggerState::Active;

        playSoundEffect( 0x97 );
        ModelItemNode::update();
    }
};
}
}
