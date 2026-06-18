#pragma once
// ERBGrouping.hpp
// Glasberg-Moore (1990) ERB-rate scale bin grouping for STFT bins.
//
// Maps NUM_BINS STFT magnitude bins into NUM_ERB_BANDS=32 ERB bands
// distributed evenly on the ERB-rate axis across [20 Hz, Nyquist/2].
//
// Public interface intentionally minimal:
//   - rebuild(sampleRate, numBins)
//   - bandStart(b) / bandEnd(b)  (half-open range [start, end))
//   - numBands()
//
// Used by FFTCompDSP as v2.2 scaffolding; detection still uses the legacy
// 4-band path until v2.3 wires C-3 / C-4 on top of this grouping.
//
// Formulas (Glasberg & Moore 1990):
//   ERB(f)   = 24.7 * (4.37 * f / 1000 + 1)
//   ERBrate  = 21.4 * log10(0.00437 * f + 1)
//   f(E)     = (10^(E / 21.4) - 1) / 0.00437

#include <cmath>
#include <algorithm>

class ERBGrouping {
public:
    static constexpr int NUM_ERB_BANDS = 32;

    ERBGrouping() {
        for (int b = 0; b < NUM_ERB_BANDS; ++b) {
            bandStart_[b] = 0;
            bandEnd_[b]   = 0;
        }
    }

    // Recompute bin ranges. Must be called whenever sample rate or numBins
    // changes. Realtime-unsafe (called from setSampleRate(), not process()).
    void rebuild(float sampleRate, int numBins) {
        if (sampleRate <= 0.f || numBins < NUM_ERB_BANDS + 1) {
            // Degenerate: assign one bin per band as fallback.
            for (int b = 0; b < NUM_ERB_BANDS; ++b) {
                bandStart_[b] = std::min(b, numBins - 1);
                bandEnd_[b]   = std::min(b + 1, numBins);
            }
            return;
        }

        const float fLo = 20.f;
        const float fHi = sampleRate * 0.25f;   // Nyquist / 2
        const float eLo = erbRate_(fLo);
        const float eHi = erbRate_(fHi);
        const float eStep = (eHi - eLo) / (float)NUM_ERB_BANDS;

        const float binHz = sampleRate / (float)((numBins - 1) * 2);
        // FFT_SIZE = (numBins - 1) * 2, bin k frequency = k * sampleRate / FFT_SIZE.

        // Compute raw integer bin index for each of the 33 ERB-rate edges.
        int edgeBin[NUM_ERB_BANDS + 1];
        for (int i = 0; i <= NUM_ERB_BANDS; ++i) {
            float E = eLo + eStep * (float)i;
            float f = erbInv_(E);
            int   k = (int)std::round(f / binHz);
            if (k < 0)               k = 0;
            if (k > numBins - 1)     k = numBins - 1;
            edgeBin[i] = k;
        }

        // Force monotonic strictly increasing so every band has >= 1 bin.
        // If pushing forward would overflow, pull subsequent edges down later.
        for (int i = 1; i <= NUM_ERB_BANDS; ++i) {
            if (edgeBin[i] <= edgeBin[i - 1]) {
                edgeBin[i] = edgeBin[i - 1] + 1;
            }
        }
        // Clamp the top: anchor the final edge at numBins - 1 (must include
        // the Nyquist bin so band 31 always terminates inside the spectrum).
        edgeBin[NUM_ERB_BANDS] = numBins - 1;
        // Walk backwards to repair any monotonic violation caused by clamping.
        for (int i = NUM_ERB_BANDS - 1; i >= 0; --i) {
            if (edgeBin[i] >= edgeBin[i + 1]) {
                edgeBin[i] = edgeBin[i + 1] - 1;
            }
            if (edgeBin[i] < 0) edgeBin[i] = 0;
        }

        for (int b = 0; b < NUM_ERB_BANDS; ++b) {
            bandStart_[b] = edgeBin[b];
            bandEnd_[b]   = edgeBin[b + 1];
        }
        // Guarantee band 31 spans through to the Nyquist bin (exclusive end
        // means numBins). Spec C-2: "band 31 結束於 numBins - 1" -> end = numBins.
        bandEnd_[NUM_ERB_BANDS - 1] = numBins;
    }

    int bandStart(int band) const { return bandStart_[band]; }
    int bandEnd(int band)   const { return bandEnd_[band]; }
    int numBands()          const { return NUM_ERB_BANDS; }

private:
    static float erbRate_(float f) {
        if (f < 0.f) f = 0.f;
        return 21.4f * std::log10(0.00437f * f + 1.f);
    }
    static float erbInv_(float E) {
        return (std::pow(10.f, E / 21.4f) - 1.f) / 0.00437f;
    }

    int bandStart_[NUM_ERB_BANDS];
    int bandEnd_[NUM_ERB_BANDS];
};
