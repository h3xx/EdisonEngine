#pragma once

#include "node.h"

namespace render::scene
{
class Scene final
{
public:
  explicit Scene() = default;

  Scene(const Scene&) = delete;

  Scene(Scene&&) = delete;

  Scene& operator=(const Scene&) = delete;

  Scene& operator=(Scene&&) = delete;

  ~Scene() = default;

  void addNode(const gsl::not_null<std::shared_ptr<Node>>& node)
  {
    if(node->m_scene == this)
    {
      // The node is already a member of this scene.
      return;
    }

    m_nodes.emplace_back(node);
    node->m_scene = this;
  }

  void accept(Visitor& visitor)
  {
    for(auto& node : m_nodes)
      visitor.visit(*node);
  }

  void clear()
  {
    auto tmp = m_nodes;
    for(const auto& node : tmp)
      setParent(node, nullptr);
  }

private:
  Node::List m_nodes;
};
} // namespace render::scene
