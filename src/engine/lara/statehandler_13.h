#pragma once

#include "statehandler_underwater.h"

namespace engine
{
    namespace lara
    {
        class StateHandler_13 final : public StateHandler_Underwater
        {
        public:
            explicit StateHandler_13(LaraNode& lara)
                    : StateHandler_Underwater(lara, LaraStateId::UnderwaterStop)
            {
            }


            void handleInputImpl(CollisionInfo& /*collisionInfo*/, const std::chrono::microseconds& deltaTime) override
            {
                if( getHealth() < 0 )
                {
                    setTargetState(LaraStateId::WaterDeath);
                }

                handleDiveInput();

                if( getLevel().m_inputHandler->getInputState().jump )
                    setTargetState(LaraStateId::UnderwaterForward);
            }

            void animateImpl(CollisionInfo& /*collisionInfo*/, const std::chrono::microseconds& deltaTimeMs) override
            {
                setFallSpeed(std::max(core::makeInterpolatedValue(0.0f), getFallSpeed() - core::makeInterpolatedValue(6.0f).getScaled(deltaTimeMs)));
            }
        };
    }
}
