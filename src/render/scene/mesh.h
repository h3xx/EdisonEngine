#pragma once

#include "multipassmaterial.h"
#include "render/gl/renderstate.h"
#include "render/gl/vertexarray.h"
#include "renderable.h"
#include "rendercontext.h"

namespace render::scene
{
class Mesh : public Renderable
{
public:
  explicit Mesh(::gl::PrimitiveType primitiveType = ::gl::PrimitiveType::Triangles)
      : m_primitiveType{primitiveType}
  {
  }

  ~Mesh() override;

  Mesh(const Mesh&) = delete;
  Mesh(Mesh&&) = delete;
  Mesh& operator=(Mesh&&) = delete;
  Mesh& operator=(const Mesh&) = delete;

  [[nodiscard]] const auto& getMaterial() const
  {
    return m_material;
  }

  [[nodiscard]] auto& getMaterial()
  {
    return m_material;
  }

  void render(RenderContext& context) final;

private:
  MultiPassMaterial m_material{};
  const ::gl::PrimitiveType m_primitiveType;

  virtual void drawIndexBuffers(::gl::PrimitiveType primitiveType) = 0;
};

template<typename IndexT, typename... VertexTs>
class MeshImpl : public Mesh
{
public:
  explicit MeshImpl(std::shared_ptr<gl::VertexArray<IndexT, VertexTs...>> vao,
                    ::gl::PrimitiveType primitiveType = ::gl::PrimitiveType::Triangles)
      : Mesh{primitiveType}
      , m_vao{std::move(vao)}
  {
  }

  ~MeshImpl() override = default;

  MeshImpl(const MeshImpl&) = delete;
  MeshImpl(MeshImpl&&) = delete;
  MeshImpl& operator=(MeshImpl&&) = delete;
  MeshImpl& operator=(const MeshImpl&) = delete;

  const auto& getVAO() const
  {
    return m_vao;
  }

private:
  gsl::not_null<std::shared_ptr<gl::VertexArray<IndexT, VertexTs...>>> m_vao;

  void drawIndexBuffers(::gl::PrimitiveType primitiveType) override
  {
    m_vao->drawIndexBuffers(primitiveType);
  }
};

extern gsl::not_null<std::shared_ptr<Mesh>>
  createQuadFullscreen(float width, float height, const gl::Program& program, bool invertY = false);
} // namespace render::scene
