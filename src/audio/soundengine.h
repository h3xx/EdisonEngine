#pragma once

#include "bufferhandle.h"
#include "sourcehandle.h"
#include "device.h"

#include "gsl-lite.hpp"

#include <map>

namespace audio
{
class SoundEngine;


class Emitter
{
    friend class SoundEngine;


public:
    explicit Emitter(const gsl::not_null<SoundEngine*>& engine)
            : m_engine{engine}
    {}

    virtual glm::vec3 getPosition() const = 0;

    virtual ~Emitter();

private:
    mutable SoundEngine* m_engine;
};


class Listener
{
    friend class SoundEngine;


public:
    explicit Listener(const gsl::not_null<SoundEngine*>& engine)
            : m_engine{engine}
    {}

    virtual glm::vec3 getPosition() const = 0;

    virtual glm::vec3 getFrontVector() const = 0;

    virtual glm::vec3 getUpVector() const = 0;

    virtual ~Listener();

private:
    mutable SoundEngine* m_engine;
};


class SoundEngine final
{
public:
    ~SoundEngine();

    void addWav(const gsl::not_null<const uint8_t*>& buffer);

    gsl::not_null<std::shared_ptr<SourceHandle>>
    playBuffer(size_t bufferId, ALfloat pitch, ALfloat volume, const glm::vec3& pos);

    gsl::not_null<std::shared_ptr<SourceHandle>>
    playBuffer(size_t bufferId, ALfloat pitch, ALfloat volume, Emitter* emitter = nullptr);

    bool stopBuffer(size_t bufferId, Emitter* emitter);

    std::vector<gsl::not_null<std::shared_ptr<SourceHandle>>>
    getSourcesForBuffer(Emitter* emitter, size_t buffer) const;

    const Device& getDevice() const noexcept
    {
        return m_device;
    }

    Device& getDevice() noexcept
    {
        return m_device;
    }

    void setListener(const Listener* listener)
    {
        m_listener = listener;
    }

    void update();

    void dropEmitter(Emitter* emitter);

private:
    Device m_device;
    std::vector<std::shared_ptr<BufferHandle>> m_buffers;
    std::map<Emitter*, std::map<size_t, std::vector<std::weak_ptr<SourceHandle>>>> m_sources;
    const Listener* m_listener = nullptr;
};
}