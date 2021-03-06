#pragma once

#include <filesystem>
#include <functional>
#include <gl/pixel.h>
#include <gl/soglb_fwd.h>

namespace SoLoud
{
class Soloud;
}

namespace video
{
extern void play(const std::filesystem::path& filename,
                 SoLoud::Soloud& soLoud,
                 const std::shared_ptr<gl::Image<gl::SRGBA8>>& img,
                 const std::function<bool()>& onFrame);
}
