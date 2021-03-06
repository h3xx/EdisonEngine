#include "finishitemanimationmenustate.h"

#include "engine/items_tr1.h"
#include "menudisplay.h"
#include "menuring.h"
#include "util.h"

namespace menu
{
std::unique_ptr<MenuState>
  FinishItemAnimationMenuState::onFrame(ui::Ui& /*ui*/, engine::World& world, MenuDisplay& display)
{
  display.updateRingTitle();

  auto& object = display.getCurrentRing().getSelectedObject();
  if(object.animate())
    return nullptr; // play full animation until its end

  if(object.type == engine::TR1ItemId::PassportOpening)
  {
    object.type = engine::TR1ItemId::PassportClosed;
    object.meshAnimFrame = 0_frame;
    object.initModel(world);
  }

  return std::move(m_next);
}

void FinishItemAnimationMenuState::handleObject(ui::Ui& /*ui*/,
                                                engine::World& /*world*/,
                                                MenuDisplay& display,
                                                MenuObject& object)
{
  if(&object != &display.getCurrentRing().getSelectedObject())
    zeroRotation(object, 256_au);
  else
    rotateForSelection(object);
}
} // namespace menu
