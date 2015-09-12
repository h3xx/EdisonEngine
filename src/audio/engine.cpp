#include "engine.h"

#include "settings.h"

#include "engine/engine.h"
#include "engine/system.h"
#include "gui/console.h"
#include "script/script.h"
#include "world/character.h"

#include <chrono>

using gui::Console;

namespace audio
{
void Engine::pauseAllSources()
{
    for(Source& source : m_sources)
    {
        if(source.isActive())
        {
            source.pause();
        }
    }
}

void Engine::stopAllSources()
{
    for(Source& source : m_sources)
    {
        source.stop();
    }
}

void Engine::resumeAllSources()
{
    for(Source& source : m_sources)
    {
        if(source.isActive())
        {
            source.play();
        }
    }
}

int Engine::getFreeSource() const  ///@FIXME: add condition (compare max_dist with new source dist)
{
    for(size_t i = 0; i < m_sources.size(); i++)
    {
        if(!m_sources[i].isActive())
        {
            return static_cast<int>(i);
        }
    }

    return -1;
}

bool Engine::endStreams(StreamType stream_type)
{
    bool result = false;

    for(StreamTrack& track : m_tracks)
    {
        if((stream_type == StreamType::Any) ||                              // End ALL streams at once.
                (track.isPlaying() &&
                 track.isType(stream_type)))
        {
            result = true;
            track.end();
        }
    }

    return result;
}

bool Engine::stopStreams(StreamType stream_type)
{
    bool result = false;

    for(StreamTrack& track : m_tracks)
    {
        if(track.isPlaying() &&
                (track.isType(stream_type) ||
                 stream_type == StreamType::Any)) // Stop ALL streams at once.
        {
            result = true;
            track.stop();
        }
    }

    return result;
}

bool Engine::isTrackPlaying(int32_t track_index) const
{
    for(const StreamTrack& track : m_tracks)
    {
        if((track_index == -1 || track.isTrack(track_index)) && track.isPlaying())
        {
            return true;
        }
    }

    return false;
}

int Engine::findSource(int effect_ID, EmitterType entity_type, int entity_ID) const
{
    for(uint32_t i = 0; i < m_sources.size(); i++)
    {
        if((entity_type == EmitterType::Any || m_sources[i].m_emitterType == entity_type) &&
                (entity_ID == -1                        || m_sources[i].m_emitterID == static_cast<int32_t>(entity_ID)) &&
                (effect_ID == -1                        || m_sources[i].m_effectIndex == static_cast<uint32_t>(effect_ID)))
        {
            if(m_sources[i].isPlaying())
                return i;
        }
    }

    return -1;
}

int Engine::getFreeStream() const
{
    for(uint32_t i = 0; i < m_tracks.size(); i++)
    {
        if(!m_tracks[i].isPlaying() && !m_tracks[i].isActive())
        {
            return i;
        }
    }

    return -1;  // If no free source, return error.
}

void Engine::updateStreams()
{
    updateStreamsDamping();

    for(StreamTrack& track : m_tracks)
    {
        track.update();
    }
}

bool Engine::trackAlreadyPlayed(uint32_t track_index, int8_t mask)
{
    if(!mask)
    {
        return false;   // No mask, play in any case.
    }

    if(track_index >= m_trackMap.size())
    {
        return true;    // No such track, hence "already" played.
    }

    mask &= 0x3F;   // Clamp mask just in case.

    if(m_trackMap[track_index] == mask)
    {
        return true;    // Immediately return true, if flags are directly equal.
    }

    int8_t played = m_trackMap[track_index] & mask;
    if(played == mask)
    {
        return true;    // Bits were set, hence already played.
    }

    m_trackMap[track_index] |= mask;
    return false;   // Not yet played, set bits and return false.
}

void Engine::updateSources()
{
    if(m_sources.empty())
    {
        return;
    }

    alGetListenerfv(AL_POSITION, m_listenerPosition);

    for(uint32_t i = 0; i < m_emitters.size(); i++)
    {
        send(m_emitters[i].sound_index, EmitterType::SoundSource, i);
    }

    for(Source& src : m_sources)
    {
        src.update();
    }
}

void Engine::updateAudio()
{
    updateSources();
    updateStreams();

    if(audio_settings.listener_is_player)
    {
        updateListenerByEntity(engine::engine_world.character);
    }
    else
    {
        updateListenerByCamera(render::renderer.camera());
    }
}

StreamError Engine::streamPlay(const uint32_t track_index, const uint8_t mask)
{
    int    target_stream = -1;
    bool   do_fade_in = false;
    StreamMethod load_method = StreamMethod::Any;
    StreamType stream_type = StreamType::Any;

    char   file_path[256];          // Should be enough, and this is not the full path...

    // Don't even try to do anything with track, if its index is greater than overall amount of
    // soundtracks specified in a stream track map count (which is derived from script).

    if(track_index >= m_trackMap.size())
    {
        Console::instance().warning(SYSWARN_TRACK_OUT_OF_BOUNDS, track_index);
        return StreamError::WrongTrack;
    }

    // Don't play track, if it is already playing.
    // This should become useless option, once proper one-shot trigger functionality is implemented.

    if(isTrackPlaying(track_index))
    {
        Console::instance().warning(SYSWARN_TRACK_ALREADY_PLAYING, track_index);
        return StreamError::Ignored;
    }

    // lua_GetSoundtrack returns stream type, file path and load method in last three
    // provided arguments. That is, after calling this function we receive stream type
    // in "stream_type" argument, file path into "file_path" argument and load method into
    // "load_method" argument. Function itself returns false, if script wasn't found or
    // request was broken; in this case, we quit.

    if(!engine_lua.getSoundtrack(track_index, file_path, &load_method, &stream_type))
    {
        Console::instance().warning(SYSWARN_TRACK_WRONG_INDEX, track_index);
        return StreamError::WrongTrack;
    }

    // Don't try to play track, if it was already played by specified bit mask.
    // Additionally, TrackAlreadyPlayed function applies specified bit mask to track map.
    // Also, bit mask is valid only for non-looped tracks, since looped tracks are played
    // in any way.

    if((stream_type != StreamType::Background) &&
            trackAlreadyPlayed(track_index, mask))
    {
        return StreamError::Ignored;
    }

    // Entry found, now process to actual track loading.

    target_stream = getFreeStream();            // At first, we need to get free stream.

    if(target_stream == -1)
    {
        do_fade_in = stopStreams(stream_type);  // If no free track found, hardly stop all tracks.
        target_stream = getFreeStream();        // Try again to assign free stream.

        if(target_stream == -1)
        {
            Console::instance().warning(SYSWARN_NO_FREE_STREAM);
            return StreamError::NoFreeStream;  // No success, exit and don't play anything.
        }
    }
    else
    {
        do_fade_in = endStreams(stream_type);   // End all streams of this type with fadeout.

        // Additionally check if track type is looped. If it is, force fade in in any case.
        // This is needed to smooth out possible pop with gapless looped track at a start-up.

        do_fade_in = (stream_type == StreamType::Background);
    }

    // Finally - load our track.

    if(!m_tracks[target_stream].load(file_path, track_index, stream_type, load_method))
    {
        Console::instance().warning(SYSWARN_STREAM_LOAD_ERROR);
        return StreamError::LoadError;
    }

    // Try to play newly assigned and loaded track.

    if(!m_tracks[target_stream].play(do_fade_in))
    {
        Console::instance().warning(SYSWARN_STREAM_PLAY_ERROR);
        return StreamError::PlayError;
    }

    return StreamError::Processed;   // Everything is OK!
}

bool Engine::deInitDelay()
{
    const std::chrono::high_resolution_clock::time_point begin_time = std::chrono::high_resolution_clock::now();

    while(isTrackPlaying() || (findSource() >= 0))
    {
        auto curr_time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - begin_time).count() / 1.0e6;

        if(curr_time > AudioDeinitDelay)
        {
            engine::Sys_DebugLog(LOG_FILENAME, "Audio deinit timeout reached! Something is wrong with audio driver.");
            break;
        }
    }

    return true;
}

void Engine::deInitAudio()
{
    stopAllSources();
    stopStreams();

    deInitDelay();

    m_sources.clear();
    m_tracks.clear();
    m_trackMap.clear();

    ///@CRITICAL: You must delete all sources before deleting buffers!

    alDeleteBuffers(static_cast<ALsizei>(m_buffers.size()), m_buffers.data());
    m_buffers.clear();

    m_effects.clear();
    m_effectMap.clear();
}

Error Engine::kill(int effect_ID, EmitterType entity_type, int entity_ID)
{
    int playing_sound = findSource(effect_ID, entity_type, entity_ID);

    if(playing_sound != -1)
    {
        m_sources[playing_sound].stop();
        return Error::Processed;
    }

    return Error::Ignored;
}

bool Engine::isInRange(EmitterType entity_type, int entity_ID, float range, float gain)
{
    btVector3 vec{ 0,0,0 };

    switch(entity_type)
    {
    case EmitterType::Entity:
        if(std::shared_ptr<world::Entity> ent = engine::engine_world.getEntityByID(entity_ID))
        {
            vec = ent->m_transform.getOrigin();
        }
        else
        {
            return false;
        }
        break;

    case EmitterType::SoundSource:
        if(static_cast<uint32_t>(entity_ID) + 1 > m_emitters.size())
        {
            return false;
        }
        vec = m_emitters[entity_ID].position;
        break;

    case EmitterType::Global:
        return true;

    default:
        return false;
    }

    auto dist = (m_listenerPosition - vec).length2();

    // We add 1/4 of overall distance to fix up some issues with
    // pseudo-looped sounds that are called at certain frames in animations.

    dist /= (gain + 1.25f);

    return dist < range * range;
}

Error Engine::send(int effect_ID, EmitterType entity_type, int entity_ID)
{
    int32_t         source_number;
    uint16_t        random_value;
    ALfloat         random_float;
    Effect* effect = nullptr;

    // If there are no audio buffers or effect index is wrong, don't process.

    if(m_buffers.empty() || effect_ID < 0)
        return Error::Ignored;

    // Remap global engine effect ID to local effect ID.

    if(static_cast<uint32_t>(effect_ID) >= m_effectMap.size())
    {
        return Error::NoSample;  // Sound is out of bounds; stop.
    }

    int real_ID = static_cast<int>(m_effectMap[effect_ID]);

    // Pre-step 1: if there is no effect associated with this ID, bypass audio send.

    if(real_ID == -1)
    {
        return Error::Ignored;
    }
    else
    {
        effect = &m_effects[real_ID];
    }

    // Pre-step 2: check if sound non-looped and chance to play isn't zero,
    // then randomly select if it should be played or not.

    if((effect->loop != loader::LoopType::Forward) && (effect->chance > 0))
    {
        random_value = rand() % 0x7FFF;
        if(effect->chance < random_value)
        {
            // Bypass audio send, if chance test is not passed.
            return Error::Ignored;
        }
    }

    // Pre-step 3: Calculate if effect's hearing sphere intersect listener's hearing sphere.
    // If it's not, bypass audio send (cause we don't want it to occupy channel, if it's not
    // heard).

    if(isInRange(entity_type, entity_ID, effect->range, effect->gain) == false)
    {
        return Error::Ignored;
    }

    // Pre-step 4: check if R (Rewind) flag is set for this effect, if so,
    // find any effect with similar ID playing for this entity, and stop it.
    // Otherwise, if W (Wait) or L (Looped) flag is set, and same effect is
    // playing for current entity, don't send it and exit function.

    source_number = findSource(effect_ID, entity_type, entity_ID);

    if(source_number != -1)
    {
        if(effect->loop == loader::LoopType::PingPong)
        {
            m_sources[source_number].stop();
        }
        else if(effect->loop != loader::LoopType::None) // Any other looping case (Wait / Loop).
        {
            return Error::Ignored;
        }
    }
    else
    {
        source_number = getFreeSource();  // Get free source.
    }

    if(source_number != -1)  // Everything is OK, we're sending audio to channel.
    {
        int buffer_index;

        // Step 1. Assign buffer to source.

        if(effect->sample_count > 1)
        {
            // Select random buffer, if effect info contains more than 1 assigned samples.
            random_value = rand() % (effect->sample_count);
            buffer_index = random_value + effect->sample_index;
        }
        else
        {
            // Just assign buffer to source, if there is only one assigned sample.
            buffer_index = effect->sample_index;
        }

        Source *source = &m_sources[source_number];

        source->setBuffer(buffer_index);

        // Step 2. Check looped flag, and if so, set source type to looped.

        if(effect->loop == loader::LoopType::Forward)
        {
            source->setLooping(AL_TRUE);
        }
        else
        {
            source->setLooping(AL_FALSE);
        }

        // Step 3. Apply internal sound parameters.

        source->m_emitterID = entity_ID;
        source->m_emitterType = entity_type;
        source->m_effectIndex = effect_ID;

        // Step 4. Apply sound effect properties.

        if(effect->rand_pitch)  // Vary pitch, if flag is set.
        {
            random_float = static_cast<ALfloat>( rand() % effect->rand_pitch_var );
            random_float = effect->pitch + ((random_float - 25.0f) / 200.0f);
            source->setPitch(random_float);
        }
        else
        {
            source->setPitch(effect->pitch);
        }

        if(effect->rand_gain)   // Vary gain, if flag is set.
        {
            random_float = static_cast<ALfloat>( rand() % effect->rand_gain_var );
            random_float = effect->gain + (random_float - 25.0f) / 200.0f;
            source->setGain(random_float);
        }
        else
        {
            source->setGain(effect->gain);
        }

        source->setRange(effect->range);    // Set audible range.

        source->play();                     // Everything is OK, play sound now!

        return Error::Processed;
    }
    else
    {
        return Error::NoChannel;
    }
}

void Engine::load(const world::World* world, const std::unique_ptr<loader::Level>& tr)
{
    // Generate new buffer array.
    m_buffers.resize(tr->m_samplesCount, 0);
    alGenBuffers(static_cast<ALsizei>(m_buffers.size()), m_buffers.data());

    // Generate stream track map array.
    // We use scripted amount of tracks to define map bounds.
    // If script had no such parameter, we define map bounds by default.
    m_trackMap.resize(engine_lua.getNumTracks(), 0);
    if(m_trackMap.empty())
        m_trackMap.resize(audio::StreamMapSize, 0);

    // Generate new audio effects array.
    m_effects.resize(tr->m_soundDetails.size());

    // Generate new audio emitters array.
    m_emitters.resize(tr->m_soundSources.size());

    // Cycle through raw samples block and parse them to OpenAL buffers.

    // Different TR versions have different ways of storing samples.
    // TR1:     sample block size, sample block, num samples, sample offsets.
    // TR2/TR3: num samples, sample offsets. (Sample block is in MAIN.SFX.)
    // TR4/TR5: num samples, (uncomp_size-comp_size-sample_data) chain.
    //
    // Hence, we specify certain parse method for each game version.

    if(!tr->m_samplesData.empty())
    {
        auto pointer = tr->m_samplesData.data();
        switch(tr->m_gameVersion)
        {
            case loader::Game::TR1:
            case loader::Game::TR1Demo:
            case loader::Game::TR1UnfinishedBusiness:
                m_effectMap = tr->m_soundmap;

                for(size_t i = 0; i < tr->m_sampleIndices.size() - 1; i++)
                {
                    pointer = tr->m_samplesData.data() + tr->m_sampleIndices[i];
                    uint32_t size = tr->m_sampleIndices[(i + 1)] - tr->m_sampleIndices[i];
                    audio::loadALbufferFromMem(m_buffers[i], pointer, size);
                }
                break;

            case loader::Game::TR2:
            case loader::Game::TR2Demo:
            case loader::Game::TR3:
            {
                m_effectMap = tr->m_soundmap;
                size_t ind1 = 0;
                size_t ind2 = 0;
                bool flag = false;
                size_t i = 0;
                while(pointer < tr->m_samplesData.data() + tr->m_samplesData.size() - 4)
                {
                    pointer = tr->m_samplesData.data() + ind2;
                    if(0x46464952 == *reinterpret_cast<int32_t*>(pointer))
                    {
                        // RIFF
                        if(!flag)
                        {
                            ind1 = ind2;
                            flag = true;
                        }
                        else
                        {
                            size_t uncomp_size = ind2 - ind1;
                            auto* srcData = tr->m_samplesData.data() + ind1;
                            audio::loadALbufferFromMem(m_buffers[i], srcData, uncomp_size);
                            i++;
                            if(i >= m_buffers.size())
                            {
                                break;
                            }
                            ind1 = ind2;
                        }
                    }
                    ind2++;
                }
                size_t uncomp_size = tr->m_samplesData.size() - ind1;
                pointer = tr->m_samplesData.data() + ind1;
                if(i < m_buffers.size())
                {
                    audio::loadALbufferFromMem(m_buffers[i], pointer, uncomp_size);
                }
                break;
            }

            case loader::Game::TR4:
            case loader::Game::TR4Demo:
            case loader::Game::TR5:
                m_effectMap = tr->m_soundmap;

                for(size_t i = 0; i < tr->m_samplesCount; i++)
                {
                    // Parse sample sizes.
                    // Always use comp_size as block length, as uncomp_size is used to cut raw sample data.
                    size_t uncomp_size = *reinterpret_cast<uint32_t*>(pointer);
                    pointer += 4;
                    size_t comp_size = *reinterpret_cast<uint32_t*>(pointer);
                    pointer += 4;

                    // Load WAV sample into OpenAL buffer.
                    audio::loadALbufferFromMem(m_buffers[i], pointer, comp_size, uncomp_size);

                    // Now we can safely move pointer through current sample data.
                    pointer += comp_size;
                }
                break;

            default:
                m_effectMap.clear();
                tr->m_samplesData.clear();
                return;
        }
    }

    // Cycle through SoundDetails and parse them into native OpenTomb
    // audio effects structure.
    for(size_t i = 0; i < m_effects.size(); i++)
    {
        if(tr->m_gameVersion < loader::Game::TR3)
        {
            m_effects[i].gain = tr->m_soundDetails[i].volume / 32767.0f; // Max. volume in TR1/TR2 is 32767.
            m_effects[i].chance = tr->m_soundDetails[i].chance;
        }
        else if(tr->m_gameVersion > loader::Game::TR3)
        {
            m_effects[i].gain = tr->m_soundDetails[i].volume / 255.0f; // Max. volume in TR3 is 255.
            m_effects[i].chance = tr->m_soundDetails[i].chance * 255;
        }
        else
        {
            m_effects[i].gain = tr->m_soundDetails[i].volume / 255.0f; // Max. volume in TR3 is 255.
            m_effects[i].chance = tr->m_soundDetails[i].chance * 127;
        }

        m_effects[i].rand_gain_var = 50;
        m_effects[i].rand_pitch_var = 50;

        m_effects[i].pitch = tr->m_soundDetails[i].pitch / 127.0f + 1.0f;
        m_effects[i].range = tr->m_soundDetails[i].sound_range * 1024.0f;

        m_effects[i].rand_pitch = tr->m_soundDetails[i].useRandomPitch();
        m_effects[i].rand_gain = tr->m_soundDetails[i].useRandomVolume();

        m_effects[i].loop = tr->m_soundDetails[i].getLoopType(loader::gameToEngine(tr->m_gameVersion));

        m_effects[i].sample_index = tr->m_soundDetails[i].sample;
        m_effects[i].sample_count = tr->m_soundDetails[i].getSampleCount();
    }

    // Try to override samples via script.
    // If there is no script entry exist, we just leave default samples.
    // NB! We need to override samples AFTER audio effects array is inited, as override
    //     routine refers to existence of certain audio effect in level.

    loadOverridedSamples(world);

    // Hardcoded version-specific fixes!

    switch(world->engineVersion)
    {
        case loader::Engine::TR1:
            // Fix for underwater looped sound.
            if(m_effectMap[TR_AUDIO_SOUND_UNDERWATER] >= 0)
            {
                m_effects[m_effectMap[TR_AUDIO_SOUND_UNDERWATER]].loop = loader::LoopType::Forward;
            }
            break;
        case loader::Engine::TR2:
            // Fix for helicopter sound range.
            if(m_effectMap[297] >= 0)
            {
                m_effects[m_effectMap[297]].range *= 10.0;
            }
            break;
        default:
            break;
    }

    // Cycle through sound emitters and
    // parse them to native OpenTomb sound emitters structure.

    for(size_t i = 0; i < m_emitters.size(); i++)
    {
        m_emitters[i].emitter_index = static_cast<ALuint>(i);
        m_emitters[i].sound_index = tr->m_soundSources[i].sound_id;
        m_emitters[i].position = btVector3( tr->m_soundSources[i].x, tr->m_soundSources[i].z, -tr->m_soundSources[i].y );
        m_emitters[i].flags = tr->m_soundSources[i].flags;
    }
}

} // namespace audio
