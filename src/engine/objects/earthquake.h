#pragma once

#include "modelobject.h"

namespace engine::objects
{
class Earthquake final : public NullRenderModelObject
{
public:
  Earthquake(const gsl::not_null<World*>& world, const core::RoomBoundPosition& position)
      : NullRenderModelObject{world, position}
  {
  }

  Earthquake(const gsl::not_null<World*>& world,
             const gsl::not_null<const loader::file::Room*>& room,
             const loader::file::Item& item,
             const gsl::not_null<const loader::file::SkeletalModelType*>& animatedModel)
      : NullRenderModelObject{world, room, item, true, animatedModel}
  {
  }

  void update() override;
};
} // namespace engine::objects
