#include "sprite.h"

#include "camera.h"
#include "material.h"
#include "mesh.h"
#include "names.h"
#include "node.h"
#include "scene.h"

#include <gl/vertexarray.h>
#include <gl/vertexbuffer.h>

namespace render::scene
{
gsl::not_null<std::shared_ptr<Mesh>> createSpriteMesh(const float x0,
                                                      const float y0,
                                                      const float x1,
                                                      const float y1,
                                                      const glm::vec2& t0,
                                                      const glm::vec2& t1,
                                                      const gsl::not_null<std::shared_ptr<Material>>& materialFull,
                                                      const int textureIdx)
{
  BOOST_ASSERT(textureIdx >= 0);

  struct SpriteVertex
  {
    glm::vec3 pos;
    glm::vec2 uv;
    int textureIdx;
    glm::vec4 color{1.0f};
    glm::vec3 normal{0, 0, 1};
  };

  const std::array<SpriteVertex, 4> vertices{SpriteVertex{{x0, y0, 0}, {t0.x, t0.y}, textureIdx},
                                             SpriteVertex{{x1, y0, 0}, {t1.x, t0.y}, textureIdx},
                                             SpriteVertex{{x1, y1, 0}, {t1.x, t1.y}, textureIdx},
                                             SpriteVertex{{x0, y1, 0}, {t0.x, t1.y}, textureIdx}};

  gl::VertexFormat<SpriteVertex> format{{VERTEX_ATTRIBUTE_POSITION_NAME, &SpriteVertex::pos},
                                        {VERTEX_ATTRIBUTE_TEXCOORD_PREFIX_NAME, &SpriteVertex::uv},
                                        {VERTEX_ATTRIBUTE_TEXINDEX_NAME, &SpriteVertex::textureIdx},
                                        {VERTEX_ATTRIBUTE_COLOR_NAME, &SpriteVertex::color},
                                        {VERTEX_ATTRIBUTE_NORMAL_NAME, &SpriteVertex::normal}};
  auto vb = std::make_shared<gl::VertexBuffer<SpriteVertex>>(format);
  vb->setData(&vertices[0], 4, gl::api::BufferUsageARB::StaticDraw);

  static const std::array<uint16_t, 6> indices{0, 1, 2, 0, 2, 3};

  auto indexBuffer = std::make_shared<gl::ElementArrayBuffer<uint16_t>>();
  indexBuffer->setData(gsl::not_null(&indices[0]), 6, gl::api::BufferUsageARB::StaticDraw);

  auto vao = std::make_shared<gl::VertexArray<uint16_t, SpriteVertex>>(
    indexBuffer, vb, std::vector{&materialFull->getShaderProgram()->getHandle()});
  auto mesh = std::make_shared<MeshImpl<uint16_t, SpriteVertex>>(vao);
  mesh->getMaterial().set(RenderMode::Full, materialFull);

  return mesh;
}

void bindSpritePole(Node& node, const SpritePole pole)
{
  node.addUniformSetter("u_spritePole",
                        [pole](const Node& /*node*/, gl::Uniform& u) { u.set(static_cast<int32_t>(pole)); });
}
} // namespace render::scene
