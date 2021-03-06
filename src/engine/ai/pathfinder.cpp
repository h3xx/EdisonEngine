#include "pathfinder.h"

#include "engine/objects/object.h"
#include "engine/world.h"
#include "serialization/box_ptr.h"
#include "serialization/deque.h"
#include "serialization/not_null.h"
#include "serialization/quantity.h"
#include "serialization/serialization.h"
#include "serialization/unordered_map.h"
#include "serialization/unordered_set.h"
#include "serialization/vector.h"

namespace engine::ai
{
PathFinder::PathFinder(const World& world)
{
  for(const auto& box : world.getBoxes())
    nodes.emplace(&box, PathFinderNode{});
}

namespace
{
gsl::span<const uint16_t> getOverlaps(const World& world, const uint16_t idx)
{
  const auto first = &world.getOverlaps().at(idx);
  auto last = first;
  const auto endOfUniverse = &world.getOverlaps().back() + 1;

  while(last < endOfUniverse && (*last & 0x8000u) == 0)
  {
    ++last;
  }

  return gsl::span{first, last + 1};
}
} // namespace

bool PathFinder::calculateTarget(const World& world, core::TRVec& moveTarget, const objects::ObjectState& objectState)
{
  updatePath(world);

  moveTarget = objectState.position.position;

  auto here = objectState.box;
  if(here == nullptr)
    return false;

  core::Length minZ = 0_len, maxZ = 0_len, minX = 0_len, maxX = 0_len;

  const auto clampX = [&minX, &maxX, &here]() {
    minX = std::max(minX, here->xmin);
    maxX = std::min(maxX, here->xmax);
  };

  const auto clampZ = [&minZ, &maxZ, &here]() {
    minZ = std::max(minZ, here->zmin);
    maxZ = std::min(maxZ, here->zmax);
  };

  constexpr uint8_t CanMoveXPos = 0x01u;
  constexpr uint8_t CanMoveXNeg = 0x02u;
  constexpr uint8_t CanMoveZPos = 0x04u;
  constexpr uint8_t CanMoveZNeg = 0x08u;
  constexpr uint8_t CanMoveAllDirs = CanMoveXPos | CanMoveXNeg | CanMoveZPos | CanMoveZNeg;
  bool detour = false;

  uint8_t moveDirs = CanMoveAllDirs;
  while(true)
  {
    if(fly != 0_len)
    {
      moveTarget.Y = std::min(moveTarget.Y, here->floor - core::SectorSize);
    }
    else
    {
      moveTarget.Y = std::min(moveTarget.Y, here->floor);
    }

    if(here->contains(objectState.position.position.X, objectState.position.position.Z))
    {
      minZ = here->zmin;
      maxZ = here->zmax;
      minX = here->xmin;
      maxX = here->xmax;
    }
    else
    {
      if(objectState.position.position.Z < here->zmin)
      {
        // try to move to -Z
        if((moveDirs & CanMoveZNeg) && here->containsX(objectState.position.position.X))
        {
          // can move straight to -Z while not leaving the X limits of the current box
          moveTarget.Z = std::max(moveTarget.Z, here->zmin + core::SectorSize / 2);

          if(detour)
            return true;

          // narrow X to the current box limits, ensure we can only move to -Z from now on
          clampX();
          moveDirs = CanMoveZNeg;
        }
        else if(detour || moveDirs != CanMoveZNeg)
        {
          moveTarget.Z = maxZ - core::SectorSize / 2;
          if(detour || moveDirs != CanMoveAllDirs)
            return true;

          detour = true;
        }
      }
      else if(objectState.position.position.Z > here->zmax)
      {
        if((moveDirs & CanMoveZPos) && here->containsX(objectState.position.position.X))
        {
          moveTarget.Z = std::min(moveTarget.Z, here->zmax - core::SectorSize / 2);

          if(detour)
            return true;

          clampX();

          moveDirs = CanMoveZPos;
        }
        else if(detour || moveDirs != CanMoveZPos)
        {
          moveTarget.Z = minZ + core::SectorSize / 2;
          if(detour || moveDirs != CanMoveAllDirs)
            return true;

          detour = true;
        }
      }

      if(objectState.position.position.X < here->xmin)
      {
        if((moveDirs & CanMoveXNeg) && here->containsZ(objectState.position.position.Z))
        {
          moveTarget.X = std::max(moveTarget.X, here->xmin + core::SectorSize / 2);

          if(detour)
            return true;

          clampZ();

          moveDirs = CanMoveXNeg;
        }
        else if(detour || moveDirs != CanMoveXNeg)
        {
          moveTarget.X = maxX - core::SectorSize / 2;
          if(detour || moveDirs != CanMoveAllDirs)
            return true;

          detour = true;
        }
      }
      else if(objectState.position.position.X > here->xmax)
      {
        if((moveDirs & CanMoveXPos) && here->containsZ(objectState.position.position.Z))
        {
          moveTarget.X = std::min(moveTarget.X, here->xmax - core::SectorSize / 2);

          if(detour)
            return true;

          clampZ();

          moveDirs = CanMoveXPos;
        }
        else if(detour || moveDirs != CanMoveXPos)
        {
          moveTarget.X = minX + core::SectorSize / 2;
          if(detour || moveDirs != CanMoveAllDirs)
            return true;

          detour = true;
        }
      }
    }

    if(here == target_box)
    {
      if(moveDirs & (CanMoveZPos | CanMoveZNeg))
      {
        moveTarget.Z = target.Z;
      }
      else if(!detour)
      {
        moveTarget.Z
          = std::clamp(moveTarget.Z, here->zmin + core::SectorSize / 2, here->zmax - core::SectorSize / 2 + 1_len);
      }
      Expects(here->containsZ(moveTarget.Z));

      if(moveDirs & (CanMoveXPos | CanMoveXNeg))
      {
        moveTarget.X = target.X;
      }
      else if(!detour)
      {
        moveTarget.X
          = std::clamp(moveTarget.X, here->xmin + core::SectorSize / 2, here->xmax - core::SectorSize / 2 + 1_len);
      }
      Expects(here->containsX(moveTarget.X));

      moveTarget.Y = target.Y;

      return true;
    }

    const auto nextBox = nodes[here].exit_box;
    if(nextBox == nullptr || !canVisit(*nextBox))
      break;

    here = nextBox;
  }

  BOOST_ASSERT(here != nullptr);
  if(moveDirs & (CanMoveZPos | CanMoveZNeg))
  {
    const auto center = here->zmax - here->zmin - core::SectorSize + 1_len;
    moveTarget.Z = util::rand15(center) + here->zmin + core::SectorSize / 2;
  }
  else if(!detour)
  {
    moveTarget.Z
      = std::clamp(moveTarget.Z, here->zmin + core::SectorSize / 2, here->zmax - core::SectorSize / 2 + 1_len);
  }
  Expects(here->containsZ(moveTarget.Z));

  if(moveDirs & (CanMoveXPos | CanMoveXNeg))
  {
    const auto center = here->xmax - here->xmin - core::SectorSize + 1_len;
    moveTarget.X = util::rand15(center) + here->xmin + core::SectorSize / 2;
  }
  else if(!detour)
  {
    moveTarget.X
      = std::clamp(moveTarget.X, here->xmin + core::SectorSize / 2, here->xmax - core::SectorSize / 2 + 1_len);
  }
  Expects(here->containsX(moveTarget.X));

  if(fly != 0_len)
    moveTarget.Y = here->floor - 384_len;
  else
    moveTarget.Y = here->floor;

  return false;
}

void PathFinder::updatePath(const World& world)
{
  if(required_box != nullptr && required_box != target_box)
  {
    target_box = required_box;

    nodes[target_box].exit_box = nullptr;
    nodes[target_box].traversable = true;
    expansions.clear();
    expansions.emplace_back(target_box);
    visited.clear();
    visited.emplace(target_box);
  }

  Expects(target_box != nullptr);
  searchPath(world);
}

void PathFinder::searchPath(const World& world)
{
  const auto zoneRef = loader::file::Box::getZoneRef(world.roomsAreSwapped(), fly, step);

  static constexpr uint8_t MaxExpansions = 5;

  for(uint8_t i = 0; i < MaxExpansions && !expansions.empty(); ++i)
  {
    const auto current = expansions.front();
    expansions.pop_front();
    const auto& currentNode = nodes[current];
    const auto searchZone = current->*zoneRef;

    for(const auto overlapBoxIdx : getOverlaps(world, current->overlap_index))
    {
      const auto* successorBox = &world.getBoxes().at(overlapBoxIdx & 0x7FFFu);

      if(successorBox == current)
        continue;

      if(searchZone != successorBox->*zoneRef)
        continue; // cannot switch zones

      const auto boxHeightDiff = successorBox->floor - current->floor;
      if(boxHeightDiff > step || boxHeightDiff < drop)
        continue; // can't reach from this box, but still maybe from another one

      auto& successorNode = nodes[successorBox];

      if(!currentNode.traversable)
      {
        if(visited.emplace(successorBox).second)
          successorNode.traversable = false;
      }
      else
      {
        if(successorNode.traversable && visited.count(successorBox) != 0)
          continue; // already visited and marked reachable

        // mark as visited and check if traversable (may switch traversable to true)
        visited.emplace(successorBox);
        successorNode.traversable = canVisit(*successorBox);
        if(successorNode.traversable)
          successorNode.exit_box = current; // success! connect both boxes

        if(std::find(expansions.begin(), expansions.end(), successorBox) == expansions.end())
          expansions.emplace_back(successorBox);
      }
    }
  }
}

void PathFinder::serialize(const serialization::Serializer<World>& ser)
{
  ser(S_NV("nodes", nodes),
      S_NV("boxes", boxes),
      S_NV("expansions", expansions),
      S_NV("visited", visited),
      S_NV("cannotVisitBlockable", cannotVisitBlockable),
      S_NV("cannotVisitBlocked", cannotVisitBlocked),
      S_NV("step", step),
      S_NV("drop", drop),
      S_NV("fly", fly),
      S_NV("targetBox", target_box),
      S_NV("requiredBox", required_box),
      S_NV("target", target));
}

void PathFinderNode::serialize(const serialization::Serializer<World>& ser)
{
  ser(S_NV("exitBox", exit_box), S_NV("traversable", traversable));
}

PathFinderNode PathFinderNode::create(const serialization::Serializer<World>& ser)
{
  PathFinderNode tmp{};
  tmp.serialize(ser);
  return tmp;
}
} // namespace engine::ai
