#pragma once
// MaskingModel.hpp
// MPEG-1 Audio Psychoacoustic Model 2 (ISO/IEC 11172-3 annex D) — simplified.
//
// Operates on ERB-band aggregated log magnitudes (dB) and produces a per-band
// masking threshold (dB) combining:
//   - Absolute Threshold of Hearing (ATH, Terhardt 1979 LUT per band center)
//   - Schroeder spreading function on ERB-rate axis
//   - Spectral Flatness Measure (SFM) tonality weighting between
//     tonal masking offset (14.5 + i dB) and noise masking offset (5.5 dB)
//
// Public interface:
//   - rebuild(sampleRate, erb)   precompute ATH + spreading LUTs (RT-unsafe)
//   - computeMaskDb(bandEnergyDb, maskDb)   per-frame mask threshold (RT-safe)
//   - athDb(band)                ATH for given band center
//
// Realtime safe: rebuild() does all allocation; computeMaskDb() only reads/writes
// preallocated buffers and does fixed arithmetic.
//
// Used by FFTCompDSP as the v2.4 (C-4) masking gate hook, gated behind
// useMasking_ flag (default off) so the v2.3 percentile detection path remains
// bit-for-bit identical when masking is disabled.

#include <cmath>
#include <vector>
#include <algorithm>
#include "ERBGrouping.hpp"

class MaskingModel {
public:
    MaskingModel() : numBands_(0) {}

    // Precompute ATH LUT per ERB band center frequency, ERB-rate per band,
    // and the spreading matrix SF[i][j] = SF(z_j - z_i).
    // Must be called from setSampleRate(), NOT from process().
    void rebuild(float sampleRate, const ERBGrouping& erb) {
        numBands_ = erb.numBands();
        ath_.assign(numBands_, 0.f);
        bandCenterHz_.assign(numBands_, 0.f);
        bandErbRate_.assign(numBands_, 0.f);
        spreadMatrix_.assign(numBands_, std::vector<float>(numBands_, 0.f));

        if (sampleRate <= 0.f || numBands_ <= 0) return;

        const int numBins = (FFT_SIZE_ASSUMED_HALF + 1);
        // Bin-Hz computed from erb_'s configured range: we ask each band for its
        // [start, end) and convert mid-bin to Hz via sampleRate / FFT_SIZE.
        // We do NOT know FFT_SIZE here, so we recover it from erb's coverage:
        // erb covers up to numBins-1 = FFT_SIZE/2 -> FFT_SIZE = 2*(numBins-1).
        // Since ERBGrouping doesn't expose numBins, we re-derive via worst case:
        // pass sampleRate and assume binHz = sampleRate / FFT_SIZE.
        // For FFTComp, FFT_SIZE = 2048 always (v2.1 constant), so we hardcode it.
        const int fftSize = 2048;
        const float binHz = sampleRate / (float)fftSize;
        (void)numBins;

        for (int b = 0; b < numBands_; ++b) {
            int s = erb.bandStart(b);
            int e = erb.bandEnd(b);
            int mid = (s + e) / 2;
            float fc = (float)mid * binHz;
            if (fc < 20.f) fc = 20.f;
            bandCenterHz_[b] = fc;
            ath_[b] = athDbFromHz_(fc);
            bandErbRate_[b] = erbRate_(fc);
        }

        // Spreading matrix: row i (source) -> col j (destination).
        // dz = z_j - z_i in ERB-rate units.
        for (int i = 0; i < numBands_; ++i) {
            for (int j = 0; j < numBands_; ++j) {
                float dz = bandErbRate_[j] - bandErbRate_[i];
                spreadMatrix_[i][j] = schroederSpread_(dz);
            }
        }
    }

    // Per-frame mask threshold computation. Inputs:
    //   bandEnergyDb : array of length numBands, ERB-band aggregated log mag
    //                  (already in dB, e.g. mean of logBuf_ within band range)
    // Outputs:
    //   maskDb       : array of length numBands, masking threshold per band
    //
    // Steps:
    //   1) Spectral Flatness Measure (SFM, dB) across bands:
    //      SFM_dB = mean(E_i) - 10*log10(mean(10^(E_i/10)))
    //      SFM ~  0 dB    : noise-like
    //      SFM ~ -60 dB   : tone-like
    //   2) tonality alpha = clamp(SFM_dB / -60, 0, 1)
    //   3) per-band offset_i = alpha*(14.5 + i) + (1-alpha)*5.5  [dB]
    //   4) mask[j] = max(ATH[j],
    //                    10*log10(sum_i 10^((E_i - offset_i + SF[i][j])/10)))
    void computeMaskDb(const float* bandEnergyDb, float* maskDb) const {
        if (numBands_ <= 0) return;

        // --- SFM (frame-global tonality estimate over the band-energy vector)
        float arithLinAccum = 0.f;
        float logAccum = 0.f;
        for (int i = 0; i < numBands_; ++i) {
            float E = bandEnergyDb[i];
            arithLinAccum += std::pow(10.f, E * 0.1f);
            logAccum += E;
        }
        float invN = 1.f / (float)numBands_;
        float arithDb = 10.f * std::log10(arithLinAccum * invN + 1e-30f);
        float geoDb   = logAccum * invN;
        float sfmDb = geoDb - arithDb;  // <= 0
        float alpha = sfmDb / -60.f;
        if (alpha < 0.f) alpha = 0.f;
        if (alpha > 1.f) alpha = 1.f;

        // --- Spreaded summation per destination band j
        for (int j = 0; j < numBands_; ++j) {
            float sumLin = 0.f;
            for (int i = 0; i < numBands_; ++i) {
                float offset_i = alpha * (14.5f + (float)i) + (1.f - alpha) * 5.5f;
                float contribDb = bandEnergyDb[i] - offset_i + spreadMatrix_[i][j];
                sumLin += std::pow(10.f, contribDb * 0.1f);
            }
            float spreadMaskDb = 10.f * std::log10(sumLin + 1e-30f);
            float m = ath_[j];
            if (spreadMaskDb > m) m = spreadMaskDb;
            maskDb[j] = m;
        }
    }

    float athDb(int band) const {
        if (band < 0 || band >= numBands_) return 0.f;
        return ath_[band];
    }
    float bandCenterHz(int band) const {
        if (band < 0 || band >= numBands_) return 0.f;
        return bandCenterHz_[band];
    }
    int numBands() const { return numBands_; }

private:
    // Terhardt (1979) Absolute Threshold of Hearing in dB SPL.
    // f in Hz. Below 20 Hz extrapolation is meaningless but clamped above.
    static float athDbFromHz_(float fHz) {
        float k = fHz * 0.001f;            // f / 1000
        if (k < 0.02f) k = 0.02f;
        float kk = k * k;
        float t1 = 3.64f * std::pow(k, -0.8f);
        float arg = (k - 3.3f);
        float t2 = -6.5f * std::exp(-0.6f * arg * arg);
        float t3 = 1e-3f * kk * kk;        // 1e-3 * k^4
        return t1 + t2 + t3;
    }

    // Schroeder spreading function (dB) vs ERB-rate distance dz.
    // dz >= 0 : upward spread (above source).
    // dz <  0 : downward spread (below source).
    static float schroederSpread_(float dz) {
        // Sign-mirrored formulation: SF should be <= 0 dB away from peak.
        // dz >= 0:  SF = 15.81 + 7.5(dz+0.474) - 17.5*sqrt(1+(dz+0.474)^2)
        // dz <  0:  SF = 15.81 - 7.5(dz-0.474) - 17.5*sqrt(1+(dz-0.474)^2)
        if (dz >= 0.f) {
            float a = dz + 0.474f;
            return 15.81f + 7.5f * a - 17.5f * std::sqrt(1.f + a * a);
        } else {
            float a = dz - 0.474f;
            return 15.81f - 7.5f * a - 17.5f * std::sqrt(1.f + a * a);
        }
    }

    static float erbRate_(float f) {
        if (f < 0.f) f = 0.f;
        return 21.4f * std::log10(0.00437f * f + 1.f);
    }

    // Marker: ERBGrouping doesn't expose its numBins; we assume FFT_SIZE 2048
    // matches FFTCompDSP::FFT_SIZE. If FFTCompDSP changes FFT_SIZE the assumed
    // constant here must follow.
    static constexpr int FFT_SIZE_ASSUMED_HALF = 1024;

    int numBands_;
    std::vector<float> ath_;
    std::vector<float> bandCenterHz_;
    std::vector<float> bandErbRate_;
    std::vector<std::vector<float>> spreadMatrix_;
};
