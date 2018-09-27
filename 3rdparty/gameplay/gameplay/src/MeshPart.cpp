#include "MeshPart.h"

#include "Material.h"

#include <utility>

namespace gameplay
{
MeshPart::MeshPart(std::shared_ptr<gl::VertexArray> vao)
        : m_vao{std::move( vao )}
{
}

MeshPart::~MeshPart() = default;

void MeshPart::draw(RenderContext& context) const
{
    if( m_material == nullptr )
        return;

    BOOST_ASSERT( context.getCurrentNode() != nullptr );

    for( const auto& mps : m_materialParameterSetters )
        mps( *context.getCurrentNode(), *m_material );

    context.pushState( m_renderState );
    context.pushState( m_material->getRenderState() );
    context.bindState();

    m_material->bind( *context.getCurrentNode() );

    m_vao->bind();

    for( const auto& buffer : m_vao->getIndexBuffers() )
    {
        buffer->draw( GL_TRIANGLES );
    }

    m_vao->unbind();
    context.popState();
    context.popState();
}
}
