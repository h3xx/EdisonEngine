#include "inflateringmenustate.h"

#include "idleringmenustate.h"
#include "menudisplay.h"
#include "menuring.h"
#include "util.h"

namespace menu
{
std::unique_ptr<MenuState> InflateRingMenuState::onFrame(ui::Ui& /*ui*/, engine::World& /*world*/, MenuDisplay& display)
{
  if(m_duration == 0_frame)
  {
    bool doAutoSelect = false;
    if(display.rings.size() == 1 && !display.passOpen && display.mode != InventoryMode::TitleMode)
    {
      doAutoSelect = true;
    }

    return create<IdleRingMenuState>(doAutoSelect);
  }

  m_duration -= 1_frame;
  m_ringTransform->ringRotation
    = display.getCurrentRing().getCurrentObjectAngle() - exactScale(90_deg, m_duration, Duration);
  m_ringTransform->cameraRotX = exactScale(m_initialCameraRotX, m_duration, Duration);
  m_ringTransform->radius += m_radiusSpeed;
  m_ringTransform->cameraPos.Y += m_cameraSpeedY;
  return nullptr;
}

void InflateRingMenuState::handleObject(ui::Ui& /*ui*/, engine::World& world, MenuDisplay& display, MenuObject& object)
{
  display.clearMenuObjectDescription();
  if(&object == &display.getCurrentRing().getSelectedObject())
  {
    idleRotation(world, object);
  }
  else
  {
    zeroRotation(object, 256_au);
  }
}

InflateRingMenuState::InflateRingMenuState(const std::shared_ptr<MenuRingTransform>& ringTransform)
    : MenuState{ringTransform}
{
}

void InflateRingMenuState::begin(engine::World& /*world*/)
{
  m_initialCameraRotX = m_ringTransform->cameraRotX;
  m_radiusSpeed = (688_len - m_ringTransform->radius) / Duration * 1_frame;
  m_cameraSpeedY = (-256_len - m_ringTransform->cameraPos.Y) / Duration * 1_frame;
}
} // namespace menu
