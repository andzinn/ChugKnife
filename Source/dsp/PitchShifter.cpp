//==============================================================================
// PitchShifter.cpp
//
// Adapted from TONE3000 (https://github.com/tone-3000/tone3000-plugin)
// Copyright (c) 2026 TONE3000.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//==============================================================================

#include "PitchShifter.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>
#include <array>

namespace {
constexpr int kMaxChannels = 2;
constexpr double kMinDelayMs = 2.0;
constexpr int kWindowMs = 30; // 30 ms default buffer (optimal for guitar and bass)
constexpr double kFadeMs = 30.0;
constexpr double kFadeMaxMs = 120.0;
constexpr double kFadeMinMs = 6.0;
constexpr double kFadeNccHi = 0.95;
constexpr double kFadeNccLo = 0.6;
constexpr double kOnsetFadeMs = 2.0;
constexpr double kMaxCorrMs = 25.0;
constexpr double kOnsetSpanMs = 4.0;
constexpr double kRefractoryMs = 40.0;
constexpr double kDetectorHpfHz = 600.0;
constexpr double kDetectorSmoothMs = 2.0;
constexpr int kHistoryCells = 50;
constexpr int kHistorySkipCells = 5;
constexpr double kOnsetOverMin = 7.94; // 9 dB over floor
constexpr double kOnsetOverMax = 3.98; // 6 dB over ceiling
constexpr double kCoarseStepRate = 12000.0;
constexpr double kSearchLeadMs = 4.0;
constexpr double kUpshiftLandShare = 0.5;
constexpr double kSearchKeepRatio = 0.1;
constexpr double kBlendSeconds = 0.025;
} // namespace

struct PitchShifter::Impl {
    struct Ring {
        std::vector<float> buf;
        uint32_t mask = 0;
        void init (int minSize) {
            int size = 1;
            while (size < minSize) size <<= 1;
            buf.assign (static_cast<size_t> (size), 0.0f);
            mask = static_cast<uint32_t> (size - 1);
        }
        void clear() { std::fill (buf.begin(), buf.end(), 0.0f); }
        void write (int64_t i, float x) { buf[static_cast<uint32_t> (i) & mask] = x; }
        float at (int64_t i) const { return buf[static_cast<uint32_t> (i) & mask]; }
        float read (double pos) const {
            const auto i = static_cast<int64_t> (std::floor (pos));
            const float t = static_cast<float> (pos - static_cast<double> (i));
            const float xm1 = at (i - 1), x0 = at (i), x1 = at (i + 1), x2 = at (i + 2);
            const float c = 0.5f * (x1 - xm1);
            const float v = x0 - x1;
            const float w = c + v;
            const float a = w + v + 0.5f * (x2 - x0);
            const float b = w + a;
            return ((a * t - b) * t + c) * t + x0;
        }
    };

    struct MirroredRing {
        std::vector<float> buf;
        uint32_t mask = 0;
        int size = 0;
        void init (int minSize) {
            size = 1;
            while (size < minSize) size <<= 1;
            buf.assign (static_cast<size_t> (size) * 2, 0.0f);
            mask = static_cast<uint32_t> (size - 1);
        }
        void clear() { std::fill (buf.begin(), buf.end(), 0.0f); }
        void write (int64_t i, float x) {
            const auto k = static_cast<uint32_t> (i) & mask;
            buf[k] = x;
            buf[k + static_cast<uint32_t> (size)] = x;
        }
        const float* windowEndingAt (int64_t i, int n) const {
            const auto k = static_cast<uint32_t> (i) & mask;
            return buf.data() + (static_cast<int> (k) + size - n);
        }
    };

    double sampleRate = 48000.0;
    float semitones = 0.0f;
    double ratio = 1.0;
    bool enabled = true;
    bool isBypassed = false;
    bool running = false;
    juce::LinearSmoothedValue<float> wetMix;
    int primeLeft = 0;

    std::array<Ring, kMaxChannels> rings;
    MirroredRing control;
    int64_t written = 0;

    int dMin = 0, dMax = 0, corrLen = 0;
    int fadeLen = 0, fadeMaxLen = 0, fadeMinLen = 0, fadeLo = 0, fadeHi = 0, onsetFadeLen = 0;
    int onsetSpan = 0, refractory = 0, cellLen = 1, coarseStep = 1, searchLead = 1, searchLeadNow = 1;

    double rA = 0.0, rB = 0.0;
    bool fading = false;
    double fade = 0.0, fadeInc = 0.0, fadeNcc = 1.0;

    struct Search {
        bool active = false, ready = false;
        int lo = 0, hi = 0, next = 0, perSample = 1;
        double bestScore = -1e9, bestNcc = 0.0;
        int bestDelay = 0;
        int64_t now0 = 0, a0 = 0;
        int64_t resultJump = 0;
    } search;
    std::vector<float> ref;
    double refXX = 0.0;

    double hpA = 0.0, hpZ1 = 0.0, hpZ2 = 0.0;
    double smoothA = 0.0, eHf = 0.0;
    std::array<float, kHistoryCells> minHist{}, maxHist{};
    double minAcc = 1e9, maxAcc = 0.0;
    int64_t lastOnset = 0;

    void updateFadeRange() {
        const double drift = std::abs (1.0 - ratio);
        double hi = fadeMaxLen;
        double lead = searchLead;
        int floor = fadeMinLen;
        if (ratio > 1.0) {
            hi = std::max<double> (fadeMinLen, std::min (hi, static_cast<double> (dMax - dMin) / (6.0 * drift)));
            const double cost = drift * (2.0 * hi + lead + 4.0) + 6.0;
            const double budget = dMax * (1.0 - kUpshiftLandShare);
            if (cost > budget) {
                const double scale = budget / cost;
                hi *= scale;
                lead *= scale;
                floor = 8;
            }
        }
        fadeHi = std::max (floor, static_cast<int> (hi));
        fadeLo = std::min (fadeLen, fadeHi);
        searchLeadNow = std::max (1, static_cast<int> (lead));
    }

    int lowGuard() const {
        return std::max (dMin, static_cast<int> (std::max (0.0, ratio - 1.0) * fadeHi) + 4);
    }

    int fadeFor (double ncc, double room) const {
        const double t = juce::jlimit (0.0, 1.0, (kFadeNccHi - ncc) / (kFadeNccHi - kFadeNccLo));
        const double len = fadeLo + t * (fadeHi - fadeLo);
        return std::max (8, static_cast<int> (std::min (len, room)));
    }

    void reset() {
        for (auto& r : rings) r.clear();
        control.clear();
        rA = rB = static_cast<double> (written - dMin);
        primeLeft = dMin + 8;
        fading = false;
        search = Search{};
        hpZ1 = hpZ2 = 0.0;
        eHf = 0.0;
        minHist.fill (1e9f);
        maxHist.fill (0.0f);
        minAcc = 1e9;
        maxAcc = 0.0;
        lastOnset = -(1 << 30);
    }

    double nccAt (int64_t b) const {
        const float* y = control.windowEndingAt (b, corrLen);
        const float* x = ref.data();
        float xy[8] = {}, yy[8] = {};
        int i = 0;
        for (; i + 8 <= corrLen; i += 8) {
            for (int k = 0; k < 8; ++k) {
                xy[k] += x[i + k] * y[i + k];
                yy[k] += y[i + k] * y[i + k];
            }
        }
        for (; i < corrLen; ++i) {
            xy[0] += x[i] * y[i];
            yy[0] += y[i] * y[i];
        }
        double sxy = 0.0, syy = 0.0;
        for (int k = 0; k < 8; ++k) {
            sxy += xy[k];
            syy += yy[k];
        }
        return sxy / (std::sqrt (refXX * syy) + 1e-9);
    }

    void beginSearch (int lo, int hi, int lead) {
        search.active = true;
        search.ready = false;
        search.now0 = written - 1;
        search.a0 = static_cast<int64_t> (std::floor (rA));
        search.lo = lo;
        search.hi = std::max (lo, hi);
        search.next = lo;
        search.bestScore = -1e9;
        search.bestDelay = lo;
        const float* x = control.windowEndingAt (search.a0, corrLen);
        std::copy (x, x + corrLen, ref.begin());
        refXX = 0.0;
        for (int i = 0; i < corrLen; ++i) refXX += static_cast<double> (x[i]) * x[i];
        const int candidates = (search.hi - search.lo) / coarseStep + 1;
        search.perSample = lead > 0 ? (candidates + lead - 1) / lead : candidates;
    }

    void consider (int d) {
        const double curDelay = static_cast<double> (search.now0 - search.a0);
        const double jump = std::max (1.0, std::abs (curDelay - static_cast<double> (d)));
        const double ncc = nccAt (search.now0 - d);
        const double s = -(1.0 - ncc) / jump;
        if (s > search.bestScore) {
            search.bestScore = s;
            search.bestDelay = d;
            search.bestNcc = ncc;
        }
    }

    void stepSearch (int count) {
        while (count-- > 0 && search.next <= search.hi) {
            consider (search.next);
            search.next += coarseStep;
        }
        if (search.next > search.hi) {
            if (coarseStep > 1) {
                const int c = search.bestDelay;
                for (int d = std::max (search.lo, c - coarseStep + 1); d <= std::min (search.hi, c + coarseStep - 1); ++d) {
                    if (d != c) consider (d);
                }
            }
            search.resultJump = (search.now0 - search.bestDelay) - search.a0;
            search.active = false;
            search.ready = true;
        }
    }

    void startFade (int64_t jump, int fadeSamples) {
        search.ready = false;
        const double destDelay = static_cast<double> (written - 1) - (rA + static_cast<double> (jump));
        if (destDelay < 2.0 || destDelay > static_cast<double> (rings[0].buf.size()) - 8.0) return;
        rB = rA + static_cast<double> (jump);
        fading = true;
        fade = 0.0;
        fadeInc = 1.0 / fadeSamples;
        fadeNcc = juce::jlimit (0.0, 1.0, search.bestNcc);
    }

    bool detectOnset (double x) {
        const int64_t now = written - 1;
        const double hp = x - hpZ1 + hpA * hpZ2;
        hpZ1 = x;
        hpZ2 = hp;
        eHf = smoothA * eHf + (1.0 - smoothA) * hp * hp;
        const int64_t cellNow = now / cellLen;
        if (now % cellLen == 0 && now > 0) {
            const auto prev = static_cast<size_t> ((cellNow - 1) % kHistoryCells);
            minHist[prev] = static_cast<float> (minAcc);
            maxHist[prev] = static_cast<float> (maxAcc);
            minAcc = 1e9;
            maxAcc = 0.0;
        }
        minAcc = std::min (minAcc, eHf);
        maxAcc = std::max (maxAcc, eHf);
        double recentMin = 1e9, recentMax = 0.0;
        for (int back = kHistorySkipCells; back < kHistoryCells; ++back) {
            const auto c = static_cast<size_t> ((cellNow - back + 4 * kHistoryCells) % kHistoryCells);
            recentMin = std::min (recentMin, static_cast<double> (minHist[c]));
            recentMax = std::max (recentMax, static_cast<double> (maxHist[c]));
        }
        return eHf > 1e-7 && eHf > recentMin * kOnsetOverMin && eHf > recentMax * kOnsetOverMax;
    }
};

PitchShifter::PitchShifter() : impl (std::make_unique<Impl>()) {}
PitchShifter::~PitchShifter() = default;

void PitchShifter::prepare (const juce::dsp::ProcessSpec& spec)
{
    auto& s = *impl;
    s.sampleRate = spec.sampleRate;
    s.dMin = static_cast<int> (s.sampleRate * kMinDelayMs * 0.001);
    s.dMax = static_cast<int> (s.sampleRate * kWindowMs * 0.001);
    s.corrLen = std::min (s.dMax, static_cast<int> (s.sampleRate * kMaxCorrMs * 0.001));

    s.fadeLen = std::max (8, static_cast<int> (s.sampleRate * kFadeMs * 0.001));
    s.fadeMaxLen = std::max (s.fadeLen, static_cast<int> (s.sampleRate * kFadeMaxMs * 0.001));
    s.fadeMinLen = std::max (8, static_cast<int> (s.sampleRate * kFadeMinMs * 0.001));
    s.onsetFadeLen = std::max (8, static_cast<int> (s.sampleRate * kOnsetFadeMs * 0.001));
    s.onsetSpan = static_cast<int> (s.sampleRate * kOnsetSpanMs * 0.001);
    s.refractory = static_cast<int> (s.sampleRate * kRefractoryMs * 0.001);
    s.searchLead = std::max (1, static_cast<int> (s.sampleRate * kSearchLeadMs * 0.001));
    s.cellLen = std::max (1, static_cast<int> (s.sampleRate * 0.001));
    s.coarseStep = std::max (1, juce::roundToInt (s.sampleRate / kCoarseStepRate));
    s.hpA = std::exp (-2.0 * juce::MathConstants<double>::pi * kDetectorHpfHz / s.sampleRate);
    s.smoothA = std::exp (-1.0 / (s.sampleRate * kDetectorSmoothMs * 0.001));

    const int maxCorr = static_cast<int> (s.sampleRate * kMaxCorrMs * 0.001);
    const int reach = static_cast<int> (s.sampleRate * 0.060) + maxCorr + s.fadeMaxLen + 2 * s.searchLead + 64;
    for (auto& r : s.rings) r.init (reach);
    s.control.init (reach);
    s.ref.assign (static_cast<size_t> (maxCorr), 0.0f);

    s.wetMix.reset (s.sampleRate, kBlendSeconds);
    s.wetMix.setCurrentAndTargetValue (0.0f);
    s.running = s.enabled;
    s.updateFadeRange();
    s.reset();
}

void PitchShifter::reset()
{
    impl->reset();
}

void PitchShifter::setSemitones (int semitones)
{
    auto& s = *impl;
    const float clamped = static_cast<float> (juce::jlimit (-24, 24, semitones));
    if (!juce::exactlyEqual (clamped, s.semitones))
    {
        s.semitones = clamped;
        const double ratio = std::pow (2.0, static_cast<double> (clamped) / 12.0);
        const auto direction = [](double r) { return r > 1.0 ? 1 : r < 1.0 ? -1 : 0; };
        if (direction (ratio) != direction (s.ratio) || std::abs (ratio - s.ratio) > kSearchKeepRatio)
            s.search = Impl::Search{};
        s.ratio = ratio;
        s.updateFadeRange();

        // If semitones == 0, smoothly blend out to dry
        const bool active = (!s.isBypassed && std::abs (s.semitones) > 0.01f);
        if (active != s.enabled)
        {
            s.enabled = active;
            s.wetMix.setTargetValue (active ? 1.0f : 0.0f);
            if (active && !s.running)
            {
                s.reset();
                s.running = true;
            }
        }
    }
}

void PitchShifter::setBypassed (bool bypassed)
{
    auto& s = *impl;
    s.isBypassed = bypassed;
    const bool active = (!s.isBypassed && std::abs (s.semitones) > 0.01f);
    if (active != s.enabled)
    {
        s.enabled = active;
        s.wetMix.setTargetValue (active ? 1.0f : 0.0f);
        if (active && !s.running)
        {
            s.reset();
            s.running = true;
        }
    }
}

float PitchShifter::getCurrentDelaySamples() const
{
    auto& s = *impl;
    if (s.isBypassed || std::abs (s.semitones) < 0.01f || !s.running)
        return 0.0f;
    return static_cast<float> ((s.dMin + s.dMax) / 2);
}

void PitchShifter::process (juce::AudioBuffer<float>& buffer)
{
    auto& s = *impl;
    const int numChannels = juce::jmin (buffer.getNumChannels(), kMaxChannels);
    const int numSamples  = buffer.getNumSamples();
    if (s.sampleRate <= 0.0 || numChannels <= 0 || numSamples <= 0 || !s.running)
        return;

    float* out[kMaxChannels];
    for (int ch = 0; ch < kMaxChannels; ++ch)
        out[ch] = buffer.getWritePointer (juce::jmin (ch, numChannels - 1));

    const float channelScale = 1.0f / static_cast<float> (numChannels);
    const double ratio = s.ratio;
    const double drift = std::abs (1.0 - ratio);
    const double leadDrift = drift * (s.searchLeadNow + 4);
    const int leadDriftI = static_cast<int> (std::ceil (leadDrift));
    const int fadeDrift = static_cast<int> (std::ceil (drift * (ratio > 1.0 ? s.fadeHi : s.fadeMinLen)));

    for (int i = 0; i < numSamples; ++i)
    {
        const int64_t now = s.written++;
        float dry[kMaxChannels];
        float control = 0.0f;
        for (int ch = 0; ch < kMaxChannels; ++ch)
        {
            dry[ch] = out[ch][i];
            s.rings[static_cast<size_t> (ch)].write (now, dry[ch]);
            if (ch < numChannels) control += dry[ch];
        }
        control *= channelScale;
        s.control.write (now, control);

        const double delay = static_cast<double> (now) - s.rA;
        const int guard = s.lowGuard();
        const bool onset = s.detectOnset (control);

        const int landLo = ratio > 1.0 ? std::min (guard + fadeDrift + leadDriftI + 2, s.dMax) : s.dMin;
        const int landHi = ratio < 1.0 ? std::max (s.dMin, s.dMax - fadeDrift - leadDriftI - 2) : s.dMax;

        if (!s.fading)
        {
            if (onset && now - s.lastOnset > s.refractory && delay > landLo + s.onsetSpan)
            {
                // Onset re-sync: pick attack brings tap to front of buffer (< 5 ms felt latency)
                s.lastOnset = now;
                s.beginSearch (landLo, std::min (landLo + s.onsetSpan, s.dMax), 0);
                s.stepSearch (s.search.perSample);
                s.startFade (s.search.resultJump, s.onsetFadeLen);
                s.minHist.fill (static_cast<float> (s.eHf));
                s.maxHist.fill (static_cast<float> (s.eHf));
                s.minAcc = s.maxAcc = s.eHf;
            }
            else if (ratio < 1.0)
            {
                // Downshift drift splice
                if (!s.search.active && !s.search.ready && delay >= s.dMax - leadDrift)
                {
                    const int lo = std::max (4, landLo - leadDriftI);
                    s.beginSearch (lo, std::max (lo, landHi - leadDriftI), s.searchLeadNow);
                }
                if (s.search.ready && delay >= s.dMax)
                {
                    const double dest = delay - static_cast<double> (s.search.resultJump);
                    const double room = (static_cast<double> (s.dMax) - dest - leadDrift - 2.0) / drift;
                    s.startFade (s.search.resultJump, s.fadeFor (s.search.bestNcc, room));
                }
            }
            else if (ratio > 1.0)
            {
                // Upshift drift splice
                if (!s.search.active && !s.search.ready && delay <= guard + leadDrift)
                    s.beginSearch (landLo + leadDriftI, landHi + leadDriftI, s.searchLeadNow);
                if (s.search.ready && delay <= guard)
                    s.startFade (s.search.resultJump, s.fadeFor (s.search.bestNcc, s.fadeHi));
            }
        }
        if (s.search.active)
            s.stepSearch (s.search.perSample);

        // Read and crossfade
        float gainA = 1.0f, gainB = 0.0f;
        if (s.fading)
        {
            gainB = static_cast<float> (0.5 - 0.5 * std::cos (juce::MathConstants<double>::pi * s.fade));
            gainA = 1.0f - gainB;
            const float r = static_cast<float> (s.fadeNcc);
            const float norm = std::sqrt (gainA * gainA + gainB * gainB + 2.0f * gainA * gainB * r);
            gainA /= norm;
            gainB /= norm;
            s.fade += s.fadeInc;
        }

        if (s.primeLeft > 0 && --s.primeLeft == 0 && s.enabled)
            s.wetMix.setTargetValue (1.0f);

        const float wetMix = s.wetMix.getNextValue();
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto& ring = s.rings[static_cast<size_t> (ch)];
            float wet = ring.read (s.rA);
            if (s.fading)
                wet = wet * gainA + gainB * ring.read (s.rB);
            out[ch][i] = dry[ch] + wetMix * (wet - dry[ch]);
        }

        if (s.fading && s.fade >= 1.0)
        {
            s.rA = s.rB;
            s.fading = false;
        }
        s.rA += ratio;
        s.rB += ratio;
    }

    if (!s.enabled && !s.wetMix.isSmoothing())
        s.running = false;
}
