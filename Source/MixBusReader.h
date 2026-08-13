/*
    MixBusReader.h
    --------------
    Reads the live spatial mix bus published by m1-system-helper
    ("M1SpatialSystem_MixBus.mem", slot-ring layout) so the monitor can decode
    a head-tracked mix even when its host bus is stereo-only (Ableton Live and
    other mono/stereo DAWs).

    Threading model:
      - a message-thread timer manages the connection (open segment, register
        consumer, drop the mapping when the helper goes away)
      - the audio thread calls read() which drains newly published blocks into
        a small jitter FIFO and hands out exactly numSamples once primed;
        under-runs surface as silence and re-prime rather than glitching
*/

#pragma once

#include <JuceHeader.h>
#include "M1MemoryShare.h"

#include <atomic>
#include <memory>

class MixBusReader : private juce::Timer
{
public:
    static constexpr const char* MIX_BUS_SEGMENT_NAME = "M1SpatialSystem_MixBus";
    static constexpr int MAX_BUS_CHANNELS = 14;

    MixBusReader()
    {
        m_consumerId = 0x4D4F4E00u | (static_cast<uint32_t>(juce::Random::getSystemRandom().nextInt(255)) & 0xFFu);
        m_fifo.setSize(MAX_BUS_CHANNELS, FIFO_CAPACITY);
        m_fifo.clear();
        startTimer(500);
    }

    ~MixBusReader() override
    {
        stopTimer();
        const juce::SpinLock::ScopedLockType lock(m_shareLock);
        if (m_share != nullptr && m_share->isValid())
            m_share->unregisterConsumer(m_consumerId);
        m_share.reset();
    }

    /** True when mix blocks arrived from the helper within the last second. */
    bool isReceiving() const
    {
        return (juce::Time::currentTimeMillis() - m_lastBlockWallMs.load()) < 1000
            && m_connected.load();
    }

    bool isConnected() const { return m_connected.load(); }
    int getBusChannels() const { return m_busChannels.load(); }
    uint32_t getBusSampleRate() const { return m_busSampleRate.load(); }

    /**
     * Audio-thread entry point. Drains available bus blocks into the jitter
     * FIFO and, once primed, copies exactly numSamples per channel into dest.
     *
     * @return number of bus channels written into dest, or 0 when no mix
     *         audio is available (caller should output silence).
     */
    int read(juce::AudioBuffer<float>& dest, int numSamples)
    {
        m_lastReadWallMs.store(juce::Time::currentTimeMillis());

        // Never block the audio thread on the connection-management lock
        const juce::SpinLock::ScopedTryLockType lock(m_shareLock);
        if (!lock.isLocked() || m_share == nullptr || !m_share->isValid())
            return 0;

        drainAvailableBlocks();

        const int channels = m_busChannels.load();
        if (channels <= 0)
            return 0;

        const uint64_t queued = m_writePos - m_readPos;

        // Prime with one extra mix block of margin so scheduling jitter
        // between the helper thread and the DAW callback doesn't starve us.
        if (!m_primed)
        {
            if (queued < static_cast<uint64_t>(numSamples + PRIME_MARGIN_SAMPLES))
                return 0;
            m_primed = true;
        }

        if (queued < static_cast<uint64_t>(numSamples))
        {
            m_primed = false; // under-run: go silent and rebuffer
            return 0;
        }

        const int outChannels = juce::jmin(channels, dest.getNumChannels());
        for (int ch = 0; ch < outChannels; ++ch)
        {
            float* out = dest.getWritePointer(ch);
            for (int i = 0; i < numSamples; ++i)
            {
                const uint64_t pos = (m_readPos + static_cast<uint64_t>(i)) % FIFO_CAPACITY;
                out[i] = m_fifo.getSample(ch, static_cast<int>(pos));
            }
        }
        m_readPos += static_cast<uint64_t>(numSamples);

        return channels;
    }

private:
    static constexpr int FIFO_CAPACITY = 16384;
    static constexpr int MAX_QUEUED_SAMPLES = 4096; // latency cap
    static constexpr int PRIME_MARGIN_SAMPLES = 512;
    // An active bus publishes at least silence keepalives every block period;
    // several seconds without traffic means the mapping is orphaned.
    static constexpr juce::int64 STALE_MAPPING_TIMEOUT_MS = 4000;

    void timerCallback() override
    {
        const bool valid = [this] {
            const juce::SpinLock::ScopedLockType lock(m_shareLock);
            return m_share != nullptr && m_share->isValid() && m_share->isRingConfigured();
        }();

        if (valid)
        {
            // Staleness check: an active helper always publishes blocks (real
            // mix or silence keepalives), so a mapping with no traffic means
            // the helper deleted and recreated the segment behind us (helper
            // restart, streaming toggle). The old mapping still LOOKS valid -
            // it points at the orphaned inode - so without this check the
            // monitor would silently decode nothing forever.
            const juce::int64 nowMs = juce::Time::currentTimeMillis();
            const juce::int64 lastActivityMs = juce::jmax(m_lastBlockWallMs.load(), m_connectedAtMs.load());
            if (nowMs - lastActivityMs <= STALE_MAPPING_TIMEOUT_MS)
                return;

            // Only recycle the mapping while the audio thread is actually
            // consuming (external decode engaged); an idle reader gains
            // nothing from reconnect churn.
            if (nowMs - m_lastReadWallMs.load() > 2000)
                return;

            DBG("[MixBusReader] No MixBus traffic - dropping mapping to reconnect");
            const juce::SpinLock::ScopedLockType lock(m_shareLock);
            m_share.reset();
            m_connected.store(false);
            m_primed = false;
            // fall through to the reconnect attempt below
        }

        // (Re)connect: open the existing segment read-side and register our
        // sequential consumer cursor.
        auto share = std::make_unique<M1MemoryShare>(MIX_BUS_SEGMENT_NAME,
                                                     16 * 1024 * 1024,
                                                     /*persistent*/ true,
                                                     /*createMode*/ false);
        if (!share->isValid() || !share->isRingConfigured())
        {
            const juce::SpinLock::ScopedLockType lock(m_shareLock);
            m_share.reset();
            m_connected.store(false);
            return;
        }

        share->registerConsumer(m_consumerId);

        const juce::SpinLock::ScopedLockType lock(m_shareLock);
        m_share = std::move(share);
        m_connected.store(true);
        m_connectedAtMs.store(juce::Time::currentTimeMillis());
        m_primed = false;
        m_writePos = 0;
        m_readPos = 0;
        DBG("[MixBusReader] Connected to MixBus segment");
    }

    // Called with m_shareLock held (audio thread).
    void drainAvailableBlocks()
    {
        M1MemoryShare::SharedBlock block;
        int drained = 0;
        while (drained < 16 && m_share->readNextBlockForConsumer(m_consumerId, block))
        {
            ++drained;

            const int numSamples = block.audio.getNumSamples();
            const int numChannels = juce::jmin(block.audio.getNumChannels(), MAX_BUS_CHANNELS);
            if (numSamples <= 0 || numChannels <= 0)
                continue;

            m_busChannels.store(numChannels);
            if (block.sampleRate != 0)
                m_busSampleRate.store(block.sampleRate);

            // Latency cap: drop oldest rather than growing monitoring delay
            const uint64_t queuedAfter = (m_writePos + static_cast<uint64_t>(numSamples)) - m_readPos;
            if (queuedAfter > static_cast<uint64_t>(MAX_QUEUED_SAMPLES))
                m_readPos = m_writePos + static_cast<uint64_t>(numSamples) - static_cast<uint64_t>(MAX_QUEUED_SAMPLES);

            for (int ch = 0; ch < numChannels; ++ch)
            {
                const float* src = block.audio.getReadPointer(ch);
                for (int i = 0; i < numSamples; ++i)
                {
                    const uint64_t pos = (m_writePos + static_cast<uint64_t>(i)) % FIFO_CAPACITY;
                    m_fifo.setSample(ch, static_cast<int>(pos), src[i]);
                }
            }
            m_writePos += static_cast<uint64_t>(numSamples);
            m_lastBlockWallMs.store(juce::Time::currentTimeMillis());
        }
    }

    juce::SpinLock m_shareLock;
    std::unique_ptr<M1MemoryShare> m_share;
    uint32_t m_consumerId = 0;

    juce::AudioBuffer<float> m_fifo;
    uint64_t m_writePos = 0;
    uint64_t m_readPos = 0;
    bool m_primed = false;

    std::atomic<bool> m_connected { false };
    std::atomic<int> m_busChannels { 0 };
    std::atomic<uint32_t> m_busSampleRate { 0 };
    std::atomic<juce::int64> m_lastBlockWallMs { 0 };
    std::atomic<juce::int64> m_connectedAtMs { 0 };
    std::atomic<juce::int64> m_lastReadWallMs { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixBusReader)
};
