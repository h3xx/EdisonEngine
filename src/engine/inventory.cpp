#include "inventory.h"

#include "engine/player.h"
#include "engine/world.h"
#include "items_tr1.h"
#include "objects/laraobject.h"
#include "serialization/map.h"
#include "serialization/serialization.h"

#include <boost/log/trivial.hpp>

namespace engine
{
void Inventory::put(const core::TypeId id, const size_t quantity)
{
  BOOST_LOG_TRIVIAL(debug) << "Object " << toString(id.get_as<TR1ItemId>()) << " added to inventory";

  switch(id.get_as<TR1ItemId>())
  {
  case TR1ItemId::PistolsSprite: [[fallthrough]];
  case TR1ItemId::Pistols: m_inventory[TR1ItemId::Pistols] = 1; break;
  case TR1ItemId::ShotgunSprite: [[fallthrough]];
  case TR1ItemId::Shotgun:
    if(const auto clips = count(TR1ItemId::ShotgunAmmoSprite))
    {
      tryTake(TR1ItemId::ShotgunAmmoSprite, clips);
      m_shotgunAmmo.ammo += 12u * clips;
    }
    m_shotgunAmmo.ammo += 12u * quantity;
    // TODO replaceItems( ShotgunSprite, ShotgunAmmoSprite );
    m_inventory[TR1ItemId::Shotgun] = 1;
    break;
  case TR1ItemId::MagnumsSprite: [[fallthrough]];
  case TR1ItemId::Magnums:
    if(const auto clips = count(TR1ItemId::MagnumAmmoSprite))
    {
      tryTake(TR1ItemId::MagnumAmmoSprite, clips);
      m_magnumsAmmo.ammo += 50u * clips;
    }
    m_magnumsAmmo.ammo += 50u * quantity;
    // TODO replaceItems( MagnumsSprite, MagnumAmmoSprite );
    m_inventory[TR1ItemId::Magnums] = 1;
    break;
  case TR1ItemId::UzisSprite: [[fallthrough]];
  case TR1ItemId::Uzis:
    if(const auto clips = count(TR1ItemId::UziAmmoSprite))
    {
      tryTake(TR1ItemId::UziAmmoSprite, clips);
      m_uzisAmmo.ammo += 100u * clips;
    }
    m_uzisAmmo.ammo += 100u * quantity;
    // TODO replaceItems( UzisSprite, UziAmmoSprite );
    m_inventory[TR1ItemId::Uzis] = 1;
    break;
  case TR1ItemId::ShotgunAmmoSprite: [[fallthrough]];
  case TR1ItemId::ShotgunAmmo:
    if(count(TR1ItemId::ShotgunSprite) > 0)
      m_shotgunAmmo.ammo += 12u;
    else
      m_inventory[TR1ItemId::ShotgunAmmo] += quantity;
    break;
  case TR1ItemId::MagnumAmmoSprite: [[fallthrough]];
  case TR1ItemId::MagnumAmmo:
    if(count(TR1ItemId::MagnumsSprite) > 0)
      m_magnumsAmmo.ammo += 50u;
    else
      m_inventory[TR1ItemId::MagnumAmmo] += quantity;
    break;
  case TR1ItemId::UziAmmoSprite: [[fallthrough]];
  case TR1ItemId::UziAmmo:
    if(count(TR1ItemId::UzisSprite) > 0)
      m_uzisAmmo.ammo += 100u;
    else
      m_inventory[TR1ItemId::UziAmmo] += quantity;
    break;
  case TR1ItemId::SmallMedipackSprite: [[fallthrough]];
  case TR1ItemId::SmallMedipack: m_inventory[TR1ItemId::SmallMedipack] += quantity; break;
  case TR1ItemId::LargeMedipackSprite: [[fallthrough]];
  case TR1ItemId::LargeMedipack: m_inventory[TR1ItemId::LargeMedipack] += quantity; break;
  case TR1ItemId::Puzzle1Sprite: [[fallthrough]];
  case TR1ItemId::Puzzle1: m_inventory[TR1ItemId::Puzzle1] += quantity; break;
  case TR1ItemId::Puzzle2Sprite: [[fallthrough]];
  case TR1ItemId::Puzzle2: m_inventory[TR1ItemId::Puzzle2] += quantity; break;
  case TR1ItemId::Puzzle3Sprite: [[fallthrough]];
  case TR1ItemId::Puzzle3: m_inventory[TR1ItemId::Puzzle3] += quantity; break;
  case TR1ItemId::Puzzle4Sprite: [[fallthrough]];
  case TR1ItemId::Puzzle4: m_inventory[TR1ItemId::Puzzle4] += quantity; break;
  case TR1ItemId::LeadBarSprite: [[fallthrough]];
  case TR1ItemId::LeadBar: m_inventory[TR1ItemId::LeadBar] += quantity; break;
  case TR1ItemId::Key1Sprite: [[fallthrough]];
  case TR1ItemId::Key1: m_inventory[TR1ItemId::Key1] += quantity; break;
  case TR1ItemId::Key2Sprite: [[fallthrough]];
  case TR1ItemId::Key2: m_inventory[TR1ItemId::Key2] += quantity; break;
  case TR1ItemId::Key3Sprite: [[fallthrough]];
  case TR1ItemId::Key3: m_inventory[TR1ItemId::Key3] += quantity; break;
  case TR1ItemId::Key4Sprite: [[fallthrough]];
  case TR1ItemId::Key4: m_inventory[TR1ItemId::Key4] += quantity; break;
  case TR1ItemId::Item141: [[fallthrough]];
  case TR1ItemId::Item148: m_inventory[TR1ItemId::Item148] += quantity; break;
  case TR1ItemId::Item142: [[fallthrough]];
  case TR1ItemId::Item149: m_inventory[TR1ItemId::Item149] += quantity; break;
  case TR1ItemId::ScionPiece1: [[fallthrough]];
  case TR1ItemId::ScionPiece2: [[fallthrough]];
  case TR1ItemId::ScionPiece5: m_inventory[TR1ItemId::ScionPiece5] += quantity; break;
  default:
    BOOST_LOG_TRIVIAL(warning) << "Cannot add object " << toString(id.get_as<TR1ItemId>()) << " to inventory";
    return;
  }
}

bool Inventory::tryUse(objects::LaraObject& lara, const TR1ItemId id)
{
  if(id == TR1ItemId::Shotgun || id == TR1ItemId::ShotgunSprite)
  {
    if(count(TR1ItemId::Shotgun) == 0)
      return false;

    lara.getWorld().getPlayer().requestedGunType = WeaponId::Shotgun;
    if(lara.getHandStatus() == objects::HandStatus::None
       && lara.getWorld().getPlayer().gunType == lara.getWorld().getPlayer().requestedGunType)
    {
      lara.getWorld().getPlayer().gunType = WeaponId::None;
    }
  }
  else if(id == TR1ItemId::Pistols || id == TR1ItemId::PistolsSprite)
  {
    if(count(TR1ItemId::Pistols) == 0)
      return false;

    lara.getWorld().getPlayer().requestedGunType = WeaponId::Pistols;
    if(lara.getHandStatus() == objects::HandStatus::None
       && lara.getWorld().getPlayer().gunType == lara.getWorld().getPlayer().requestedGunType)
    {
      lara.getWorld().getPlayer().gunType = WeaponId::None;
    }
  }
  else if(id == TR1ItemId::Magnums || id == TR1ItemId::MagnumsSprite)
  {
    if(count(TR1ItemId::Magnums) == 0)
      return false;

    lara.getWorld().getPlayer().requestedGunType = WeaponId::Magnums;
    if(lara.getHandStatus() == objects::HandStatus::None
       && lara.getWorld().getPlayer().gunType == lara.getWorld().getPlayer().requestedGunType)
    {
      lara.getWorld().getPlayer().gunType = WeaponId::None;
    }
  }
  else if(id == TR1ItemId::Uzis || id == TR1ItemId::UzisSprite)
  {
    if(count(TR1ItemId::Uzis) == 0)
      return false;

    lara.getWorld().getPlayer().requestedGunType = WeaponId::Uzis;
    if(lara.getHandStatus() == objects::HandStatus::None
       && lara.getWorld().getPlayer().gunType == lara.getWorld().getPlayer().requestedGunType)
    {
      lara.getWorld().getPlayer().gunType = WeaponId::None;
    }
  }
  else if(id == TR1ItemId::LargeMedipack || id == TR1ItemId::LargeMedipackSprite)
  {
    if(count(TR1ItemId::LargeMedipack) == 0)
      return false;

    if(lara.isDead() || lara.m_state.health >= core::LaraHealth)
    {
      return false;
    }

    lara.m_state.health = std::min(lara.m_state.health + core::LaraHealth, core::LaraHealth);
    tryTake(TR1ItemId::LargeMedipackSprite);
    lara.playSoundEffect(TR1SoundEffect::LaraSigh);
  }
  else if(id == TR1ItemId::SmallMedipack || id == TR1ItemId::SmallMedipackSprite)
  {
    if(count(TR1ItemId::SmallMedipack) == 0)
      return false;

    if(lara.isDead() || lara.m_state.health >= core::LaraHealth)
    {
      return false;
    }

    lara.m_state.health = std::min(lara.m_state.health + core::LaraHealth / 2, core::LaraHealth);
    tryTake(TR1ItemId::SmallMedipackSprite);
    lara.playSoundEffect(TR1SoundEffect::LaraSigh);
  }

  return true;
}

bool Inventory::tryTake(const TR1ItemId id, const size_t quantity)
{
  BOOST_LOG_TRIVIAL(debug) << "Taking object " << toString(id) << " from inventory";

  const auto it = m_inventory.find(id);
  if(it == m_inventory.end())
    return false;

  if(it->second < quantity)
    return false;

  if(it->second == quantity)
    m_inventory.erase(it);
  else
    m_inventory[id] -= quantity;

  return true;
}

void Inventory::serialize(const serialization::Serializer<World>& ser)
{
  ser(S_NV("inventory", m_inventory),
      S_NV("pistolsAmmo", m_pistolsAmmo),
      S_NV("magnumsAmmo", m_magnumsAmmo),
      S_NV("uzisAmmo", m_uzisAmmo),
      S_NV("shotgunAmmo", m_shotgunAmmo));
}

void Ammo::serialize(const serialization::Serializer<World>& ser)
{
  ser(S_NV("ammo", ammo), S_NV("hits", hits), S_NV("misses", misses));
}
} // namespace engine
