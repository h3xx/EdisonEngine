#include "scionpiece.h"

#include "engine/laranode.h"

namespace engine
{
namespace items
{
void ScionPieceItem::collide(LaraNode& lara, CollisionInfo& /*collisionInfo*/)
{
    static const InteractionLimits limits{
            core::BoundingBox{{-256, 540, -350},
                              {256,  740, -200}},
            {-10_deg, 0_deg, 0_deg},
            {10_deg, 0_deg, 0_deg}
    };

    m_state.rotation.X = 0_deg;
    m_state.rotation.Y = lara.m_state.rotation.Y;
    m_state.rotation.Z = 0_deg;

    if( !limits.canInteract( m_state, lara.m_state ) )
        return;

    if( lara.getCurrentAnimState() != loader::LaraStateId::PickUp )
    {
        if( getLevel().m_inputHandler->getInputState().action
            && lara.getHandStatus() == engine::HandStatus::None
            && !lara.m_state.falling
            && lara.getCurrentAnimState() == loader::LaraStateId::Stop )
        {
            lara.alignForInteraction( {0, 640, -310}, m_state );
            lara.m_state.anim = getLevel().findAnimatedModelForType( engine::TR1ItemId::AlternativeLara )
                                          ->animation;
            lara.m_state.current_anim_state = 39;
            lara.setCurrentAnimState( loader::LaraStateId::PickUp );
            lara.setGoalAnimState( loader::LaraStateId::PickUp );
            lara.m_state.frame_number = lara.m_state.anim->firstFrame;
            getLevel().m_cameraController->setMode( engine::CameraMode::Cinematic );
            lara.setHandStatus( engine::HandStatus::Grabbing );
            getLevel().m_cameraController->m_cinematicFrame = 0;
            getLevel().m_cameraController->m_cinematicPos = lara.m_state.position.position;
            getLevel().m_cameraController->m_cinematicRot = lara.m_state.rotation;
        }
    }
    else if( lara.m_state.frame_number == lara.m_state.anim->firstFrame + 44 )
    {
        getLevel().addInventoryItem( m_state.object_number );
        m_state.triggerState = engine::items::TriggerState::Invisible;
        getNode()->setVisible( false );
    }
}
}
}