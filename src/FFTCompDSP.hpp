#pragma once
// FFTCompDSP.hpp
// Spectral resonance suppression (ANINA-style) for VCV Rack module FFTComp.
//
// Notes:
// - MONO SIMPLIFIED FIRST VERSION:
//     L+R are averaged into a single FFT pipeline for processing.
//     The same per-bin gain reduction is applied to both L and R outputs.
//     Sidechain L+R are also averaged for detection.
// - FFT library: rack::dsp::RealFFT (PFFFT wrapper, ships with Rack SDK).
// - Realtime safe: no malloc / new / lock inside process().
//   All buffers are allocated in setSampleRate() / constructor.
// - Latency: ~ FFT_SIZE samples (OLA). Module reports as effect type.
// - API matches the spec exactly. UI agent interface compatible.
//
// Signal flow (per FFT frame):
//   accumulate -> window -> RFFT -> PreEQ(magnitude) -> per-bin prominence
//   -> per-bin band-weight * amount -> targetGR -> attack/release smoothing
//   -> apply GR -> PostEQ(magnitude) -> iRFFT -> window -> OLA
//   -> mix dry/wet -> makeup gain

#include <rack.hpp>
#include <cstring>
#include <cmath>
#include <vector>
#include <algorithm>
#include "ERBGrouping.hpp"
#include "MaskingModel.hpp"

class FFTCompDSP {
public:
    static constexpr int FFT_SIZE = 2048;
    static constexpr int HOP_SIZE = 256;            // v2.1.1: standard 75% overlap dual-window OLA
    static constexpr int SYN_LEN  = 1024;           // v2.1.1: sqrt-Hann length; SYN_LEN/HOP_SIZE = 4x overlap (COLA)
    static constexpr int OUT_RING_SIZE = 2048;      // v2.1.1: >= SYN_LEN + HOP_SIZE; power of 2 for safety
    static constexpr int NUM_BANDS = 4;
    static constexpr int NUM_BINS = FFT_SIZE / 2 + 1;

    // {LO shelf, LMID bell, HMID bell, HI shelf}
    static constexpr float DEFAULT_FREQS[4] = {180.f, 350.f, 1100.f, 3000.f};

    FFTCompDSP()
        : sampleRate_(48000.f),
          rfft_(FFT_SIZE),
          writePos_(0),
          hopCounter_(0),
          readPos_(0),
          dryWritePos_(0),
          attackNorm_(0.2f),
          releaseNorm_(0.4f),
          attackCoef_(0.5f),
          releaseCoef_(0.1f),
          mix_(1.0f),
          makeupDb_(0.0f),
          makeupGain_(1.0f),
          olaGain_(1.f)   // v2.1: recomputed in constructor from window pair
    {
        for (int b = 0; b < NUM_BANDS; ++b) {
            bandFreq_[b]    = DEFAULT_FREQS[b];
            bandPreDb_[b]   = 0.f;
            bandPostDb_[b]  = 0.f;
            bandAmount_[b]  = 0.f;
        }

        // ring + scratch buffers
        // v2.1.1: output rings = OUT_RING_SIZE (2048). dry delays = SYN_LEN (1024)
        //         so algorithmic latency of wet/SC monitor/dry are all SYN_LEN samples.
        inputRing_.assign(FFT_SIZE, 0.f);
        scRing_.assign(FFT_SIZE, 0.f);
        outputRing_.assign(OUT_RING_SIZE, 0.f);
        scMonRing_.assign(OUT_RING_SIZE, 0.f);
        dryDelayL_.assign(SYN_LEN, 0.f);
        dryDelayR_.assign(SYN_LEN, 0.f);

        fftTimeIn_.assign(FFT_SIZE, 0.f);
        fftFreq_.assign(FFT_SIZE, 0.f);
        fftScTimeIn_.assign(FFT_SIZE, 0.f);
        fftScFreq_.assign(FFT_SIZE, 0.f);
        fftTimeOut_.assign(FFT_SIZE, 0.f);
        fftScTimeOut_.assign(FFT_SIZE, 0.f);

        magMain_.assign(NUM_BINS, 0.f);
        magSc_.assign(NUM_BINS, 0.f);
        logBuf_.assign(NUM_BINS, 0.f);
        smoothedLog_.assign(NUM_BINS, 0.f);
        smoothedGR_.assign(NUM_BINS, 1.f);
        displayMag_.assign(NUM_BINS, 0.f);
        displayGR_.assign(NUM_BINS, 1.f);

        // v2.3: ERB-band noise-floor scratch + IIR state.
        // floorSmoothedDb_ holds the per-band IIR-smoothed 75th percentile floor
        // in dB. Initialized to -120 dB (silence floor) so the first frame's IIR
        // ramp does not introduce a spurious GR transient.
        erbScratch_.assign(NUM_BINS, 0.f);
        floorSmoothedDb_.assign(ERBGrouping::NUM_ERB_BANDS, -120.f);

        // v2.4 (C-4): masking model state.
        // bandEnergyDb_ holds per-frame ERB-band mean log mag (dB).
        // maskDb_ holds the masking threshold (dB) returned by MaskingModel.
        // bandOfBin_ maps each STFT bin index to its owning ERB band, populated
        // once erb_ is rebuilt (see setSampleRate / recomputeAll_).
        bandEnergyDb_.assign(ERBGrouping::NUM_ERB_BANDS, -120.f);
        maskDb_.assign(ERBGrouping::NUM_ERB_BANDS, -120.f);
        bandOfBin_.assign(NUM_BINS, 0);

        // v2.5 (C-5): group delay scratch + per-band GD floor (dB).
        // phaseBuf_ stores unwrapped SC phase per bin (workspace, no cross-frame
        // state — phase unwrap is per-frame only).
        // gdBuf_ stores |GD(k)| (normalized, sample units) after diff+clamp.
        // gdFloorBuf_ stores per-band 75th percentile of log|GD| (dB).
        phaseBuf_.assign(NUM_BINS, 0.f);
        gdBuf_.assign(NUM_BINS, 0.f);
        gdFloorBuf_.assign(ERBGrouping::NUM_ERB_BANDS, -120.f);

        // Analysis window: Hann length FFT_SIZE (unchanged)
        window_.assign(FFT_SIZE, 0.f);
        for (int n = 0; n < FFT_SIZE; ++n) {
            window_[n] = 0.5f * (1.f - std::cos(2.f * M_PI * n / (FFT_SIZE - 1)));
        }

        // v2.1: synthesis window — sqrt-Hann length SYN_LEN, zero-padded.
        // Placed at the tail of the analysis buffer so it covers the most-recent
        // SYN_LEN input samples. This gives algorithmic latency = HOP_SIZE.
        synWindow_.assign(FFT_SIZE, 0.f);
        for (int k = 0; k < SYN_LEN; ++k) {
            float h = 0.5f * (1.f - std::cos(2.f * M_PI * k / (SYN_LEN - 1)));
            synWindow_[FFT_SIZE - SYN_LEN + k] = std::sqrt(h);
        }

        // v2.1.1: OLA gain — proper COLA normalization.
        // With SYN_LEN=1024 and HOP_SIZE=256 (4x overlap), each output sample receives
        // contributions from SYN_LEN/HOP_SIZE = 4 frames at synthesis-window positions
        // k, k+H, k+2H, k+3H. The reconstruction "constant" is
        //     S(k) = sum_{m=0..3} w_a[base+k+m*H] * w_s[base+k+m*H]   for k in [0, H).
        // For Hann_2048 (descending half) * sqrt-Hann_1024 this is nearly (but not
        // exactly) constant; we use the mean over the hop as the normalizer so the
        // residual ripple averages out and overall level is unity-gain.
        {
            const int frameBase = FFT_SIZE - SYN_LEN;
            const int nOverlap = SYN_LEN / HOP_SIZE;   // = 4
            double accum = 0.0;
            for (int k = 0; k < HOP_SIZE; ++k) {
                double Sk = 0.0;
                for (int m = 0; m < nOverlap; ++m) {
                    int idx = frameBase + k + m * HOP_SIZE;
                    Sk += (double)window_[idx] * (double)synWindow_[idx];
                }
                accum += Sk;
            }
            double meanS = accum / (double)HOP_SIZE;
            olaGain_ = (meanS > 1e-12) ? (float)(1.0 / meanS) : 1.f;
        }

        // EQ gain tables
        preGainBins_.assign(NUM_BINS, 1.f);
        postGainBins_.assign(NUM_BINS, 1.f);
        scFilterGainBins_.assign(NUM_BINS, 1.f);
        for (int b = 0; b < NUM_BANDS; ++b) bandShape_[b].assign(NUM_BINS, 0.f);
        weightBins_.assign(NUM_BINS, 0.f);

        // SC HPF/LPF defaults: OFF
        scHpfHz_ = 20.f;
        scLpfHz_ = 20000.f;

        recomputeAll_();
        recomputeAttackRelease_();
    }

    void setSampleRate(float sr) {
        if (sr <= 0.f) return;
        if (sampleRate_ == sr) return;
        sampleRate_ = sr;
        erb_.rebuild(sampleRate_, NUM_BINS);
        // v2.3: reset noise-floor IIR state on SR change so we don't carry over
        // an outdated dB level into the new band layout.
        std::fill(floorSmoothedDb_.begin(), floorSmoothedDb_.end(), -120.f);
        // v2.4: rebuild masking LUTs and binToBand map for new SR.
        masking_.rebuild(sampleRate_, erb_);
        recomputeBandOfBin_();
        recomputeAll_();
        recomputeAttackRelease_();
    }

    // v2.2: enable ERB-band detection path. Off by default; legacy 4-band
    // weightBins_ path remains active until v2.3 wires C-3/C-4.
    void setUseERB(bool e) { useERB_ = e; }

    // v2.4 (C-4): enable psychoacoustic masking gate. Default off; when off the
    // detection path is bit-for-bit identical to v2.3. Requires useERB_=true
    // (masking operates on ERB-band aggregates), otherwise it is a no-op.
    void setUseMasking(bool m) { useMasking_ = m; }

    // v2.5 (C-5): enable group delay peak picking. Off by default; when off the
    // detection path is bit-for-bit identical to v2.4. Requires useERB_=true
    // (GD floor is computed per ERB band), otherwise it is a no-op.
    void setUseGroupDelay(bool g) { useGroupDelay_ = g; }
    void setAlphaGd(float a) { alphaGd_ = a; }

    void setBandFrequency(int band, float hz) {
        if (band < 0 || band >= NUM_BANDS) return;
        if (hz < 20.f) hz = 20.f;
        float maxHz = sampleRate_ * 0.45f;
        if (hz > maxHz) hz = maxHz;
        bandFreq_[band] = hz;
        recomputeBandShape_(band);
        recomputeWeightBins_();
        recomputePreGainBins_();
        recomputePostGainBins_();
    }

    void setPreEqGain(int band, float dB) {
        if (band < 0 || band >= NUM_BANDS) return;
        if (dB < -24.f) dB = -24.f;
        if (dB >  24.f) dB =  24.f;
        if (bandPreDb_[band] == dB) return;
        bandPreDb_[band] = dB;
        recomputePreGainBins_();
    }

    void setAmount(int band, float amt01) {
        if (band < 0 || band >= NUM_BANDS) return;
        if (amt01 < 0.f) amt01 = 0.f;
        if (amt01 > 1.f) amt01 = 1.f;
        if (bandAmount_[band] == amt01) return;
        bandAmount_[band] = amt01;
        recomputeWeightBins_();
    }

    void setAttack(float norm01) {
        if (norm01 < 0.f) norm01 = 0.f;
        if (norm01 > 1.f) norm01 = 1.f;
        if (attackNorm_ == norm01) return;
        attackNorm_ = norm01;
        recomputeAttackRelease_();
    }

    void setRelease(float norm01) {
        if (norm01 < 0.f) norm01 = 0.f;
        if (norm01 > 1.f) norm01 = 1.f;
        if (releaseNorm_ == norm01) return;
        releaseNorm_ = norm01;
        recomputeAttackRelease_();
    }

    void setMix(float wet01) {
        if (wet01 < 0.f) wet01 = 0.f;
        if (wet01 > 1.f) wet01 = 1.f;
        if (mix_ == wet01) return;
        mix_ = wet01;
    }

    void setMakeupGain(float dB) {
        if (dB < -12.f) dB = -12.f;
        if (dB >  12.f) dB =  12.f;
        if (makeupDb_ == dB) return;
        makeupDb_ = dB;
        makeupGain_ = std::pow(10.f, dB / 20.f);
    }

    void setScHpfFreq(float hz) {
        if (hz < 20.f) hz = 20.f;
        if (hz > 2000.f) hz = 2000.f;
        if (scHpfHz_ == hz) return;
        scHpfHz_ = hz;
        recomputeScFilterBins_();
    }

    void setScLpfFreq(float hz) {
        if (hz < 200.f) hz = 200.f;
        float maxHz = sampleRate_ * 0.45f;
        if (hz > 20000.f) hz = 20000.f;
        if (hz > maxHz) hz = maxHz;
        if (scLpfHz_ == hz) return;
        scLpfHz_ = hz;
        recomputeScFilterBins_();
    }

    void setPostEqGain(int band, float dB) {
        if (band < 0 || band >= NUM_BANDS) return;
        if (dB < -24.f) dB = -24.f;
        if (dB >  24.f) dB =  24.f;
        if (bandPostDb_[band] == dB) return;
        bandPostDb_[band] = dB;
        recomputePostGainBins_();
    }

    void process(float inL, float inR, float scL, float scR,
                 float& outL, float& outR, float& scMonL, float& scMonR)
    {
        // store dry delay
        dryDelayL_[dryWritePos_] = inL;
        dryDelayR_[dryWritePos_] = inR;

        // mono mix into FFT input rings
        float mainMono = 0.5f * (inL + inR);
        float scMono   = 0.5f * (scL + scR);
        inputRing_[writePos_] = mainMono;
        scRing_[writePos_]    = scMono;

        writePos_    = (writePos_ + 1) % FFT_SIZE;
        dryWritePos_ = (dryWritePos_ + 1) % SYN_LEN;    // v2.1.1: dry latency = SYN_LEN (1024)
        hopCounter_++;

        if (hopCounter_ >= HOP_SIZE) {
            hopCounter_ = 0;
            processFrame_();
        }

        // consume one OLA sample (main wet + SC monitor share readPos_)
        float wetMono = outputRing_[readPos_];
        outputRing_[readPos_] = 0.f;
        float scMonMono = scMonRing_[readPos_];
        scMonRing_[readPos_] = 0.f;
        readPos_ = (readPos_ + 1) % OUT_RING_SIZE;     // v2.1.1: ring size = OUT_RING_SIZE (2048)

        // v2.1.1: dry aligned to SYN_LEN latency (1024).
        // Oldest sample in dry ring sits at dryWritePos_ after the post-increment.
        float dryL = dryDelayL_[dryWritePos_];
        float dryR = dryDelayR_[dryWritePos_];

        outL = (dryL * (1.f - mix_) + wetMono * mix_) * makeupGain_;
        outR = (dryR * (1.f - mix_) + wetMono * mix_) * makeupGain_;

        // SC monitor: filtered sidechain, mono -> both channels.
        // Naturally SYN_LEN delayed to align with main wet path.
        scMonL = scMonMono;
        scMonR = scMonMono;
    }

    const float* getSpectrumMagnitudes() const  { return displayMag_.data(); }
    const float* getGainReductionPerBin() const { return displayGR_.data(); }
    int getNumBins() const { return NUM_BINS; }

private:
    void processFrame_() {
        // assemble windowed frame from ring (oldest first = current writePos_)
        int start = writePos_;
        for (int n = 0; n < FFT_SIZE; ++n) {
            int idx = (start + n) % FFT_SIZE;
            fftTimeIn_[n]   = inputRing_[idx] * window_[n];
            fftScTimeIn_[n] = scRing_[idx]    * window_[n];
        }

        rfft_.rfft(fftTimeIn_.data(),   fftFreq_.data());
        rfft_.rfft(fftScTimeIn_.data(), fftScFreq_.data());

        // ordered RFFT layout:
        //   [0]   = F(0) (DC, real)
        //   [1]   = F(N/2) (Nyquist, real)
        //   [2k]  = real(F(k))
        //   [2k+1]= imag(F(k))  for k = 1..N/2-1
        magMain_[0]            = std::fabs(fftFreq_[0]);
        magMain_[NUM_BINS - 1] = std::fabs(fftFreq_[1]);
        magSc_[0]              = std::fabs(fftScFreq_[0]);
        magSc_[NUM_BINS - 1]   = std::fabs(fftScFreq_[1]);
        for (int k = 1; k < NUM_BINS - 1; ++k) {
            float re = fftFreq_[2 * k];
            float im = fftFreq_[2 * k + 1];
            magMain_[k] = std::sqrt(re * re + im * im);

            float sre = fftScFreq_[2 * k];
            float sim = fftScFreq_[2 * k + 1];
            magSc_[k] = std::sqrt(sre * sre + sim * sim);
        }

        // SC HPF/LPF on SC magnitudes AND complex coefficients (for monitor IFFT).
        // Applied BEFORE Pre EQ so SC filter only affects sidechain path.
        {
            float gDC = scFilterGainBins_[0];
            fftScFreq_[0] *= gDC;
            magSc_[0]     *= gDC;
            float gNy = scFilterGainBins_[NUM_BINS - 1];
            fftScFreq_[1] *= gNy;
            magSc_[NUM_BINS - 1] *= gNy;
            for (int k = 1; k < NUM_BINS - 1; ++k) {
                float g = scFilterGainBins_[k];
                fftScFreq_[2 * k]     *= g;
                fftScFreq_[2 * k + 1] *= g;
                magSc_[k] *= g;
            }
        }

        // IFFT filtered SC into monitor OLA ring (independent of main wet path).
        rfft_.irfft(fftScFreq_.data(), fftScTimeOut_.data());
        rfft_.scale(fftScTimeOut_.data());
        // v2.1.1: write only the SYN_LEN samples in [FFT_SIZE-SYN_LEN, FFT_SIZE)
        //         weighted by synWindow_ (sqrt-Hann tail). 4x overlap accumulates
        //         into outputRing_ via += ; samples are cleared only after being read.
        {
            int olaStart = readPos_;
            const int ringSz = OUT_RING_SIZE;
            const int frameBase = FFT_SIZE - SYN_LEN;
            for (int k = 0; k < SYN_LEN; ++k) {
                int n = frameBase + k;
                int idx = (olaStart + k) % ringSz;
                scMonRing_[idx] += fftScTimeOut_[n] * synWindow_[n] * olaGain_;
            }
        }

        // Pre EQ on magnitudes (applies in spectral domain).
        // Main and SC both pre-EQ'd so detection sees the same shaping.
        for (int k = 0; k < NUM_BINS; ++k) {
            magMain_[k] *= preGainBins_[k];
            magSc_[k]   *= preGainBins_[k];
        }

        // log magnitudes of SC for prominence detection
        constexpr float MIN_LIN = 1e-9f;
        for (int k = 0; k < NUM_BINS; ++k) {
            float m = magSc_[k];
            if (m < MIN_LIN) m = MIN_LIN;
            logBuf_[k] = 20.f * std::log10(m);
        }

        if (!useERB_) {
            // Legacy v2.1+v2.2 path: local symmetric moving average in log domain
            // (+/- SMOOTH_N bins). Kept verbatim so disabling the ERB flag returns
            // to known-good behavior bit-for-bit.
            constexpr int SMOOTH_N = 9;
            for (int k = 0; k < NUM_BINS; ++k) {
                int lo = k - SMOOTH_N; if (lo < 0) lo = 0;
                int hi = k + SMOOTH_N; if (hi > NUM_BINS - 1) hi = NUM_BINS - 1;
                float sum = 0.f;
                for (int j = lo; j <= hi; ++j) sum += logBuf_[j];
                smoothedLog_[k] = sum / (hi - lo + 1);
            }
        } else {
            // v2.3 (C-3): Adaptive percentile prominence floor per ERB band.
            // For each ERB band:
            //   - copy band's logBuf_ slice into erbScratch_
            //   - 75th percentile via std::nth_element (O(n) avg) as the
            //     band noise floor in dB
            //   - if band has < 4 bins, fall back to band mean (percentile
            //     unstable on tiny N)
            //   - 1st-order IIR temporal smoothing on floor_dB so cross-frame
            //     percentile jumps don't shake GR
            //   - broadcast smoothed floor_dB back into smoothedLog_[k] for every
            //     bin k in the band, so the downstream
            //         prom_dB = logBuf_[k] - smoothedLog_[k] - promThreshold
            //     formula stays untouched.
            const int nBands = erb_.numBands();
            const float a = floorSmoothAlpha_;
            for (int b = 0; b < nBands; ++b) {
                int s = erb_.bandStart(b);
                int e = erb_.bandEnd(b);
                int n = e - s;
                if (n <= 0) continue;

                float floor_dB;
                if (n < 4) {
                    float sum = 0.f;
                    for (int k = s; k < e; ++k) sum += logBuf_[k];
                    floor_dB = sum / (float)n;
                } else {
                    for (int k = s; k < e; ++k) erbScratch_[k - s] = logBuf_[k];
                    int idx = (int)std::floor(0.75f * (float)n);
                    if (idx < 0) idx = 0;
                    if (idx >= n) idx = n - 1;
                    auto first = erbScratch_.begin();
                    std::nth_element(first, first + idx, first + n);
                    floor_dB = erbScratch_[idx];
                }

                float prev = floorSmoothedDb_[b];
                float sm = a * floor_dB + (1.f - a) * prev;
                floorSmoothedDb_[b] = sm;

                for (int k = s; k < e; ++k) smoothedLog_[k] = sm;
            }

            // v2.4 (C-4): masking threshold per ERB band (optional).
            // Requires useERB_; only runs when useMasking_ is also on. Computes
            //   bandEnergyDb_[b] = mean(logBuf_[k]) for k in band b
            //   maskDb_[b]       = MaskingModel(bandEnergyDb_)
            // The per-bin GR loop below subtracts maskDb_[bandOfBin_[k]] from
            // prom_dB so masked bins drop out of detection.
            if (useMasking_) {
                for (int b = 0; b < nBands; ++b) {
                    int s = erb_.bandStart(b);
                    int e = erb_.bandEnd(b);
                    int n = e - s;
                    if (n <= 0) { bandEnergyDb_[b] = -120.f; continue; }
                    float sum = 0.f;
                    for (int k = s; k < e; ++k) sum += logBuf_[k];
                    bandEnergyDb_[b] = sum / (float)n;
                }
                masking_.computeMaskDb(bandEnergyDb_.data(), maskDb_.data());
            }
        }

        // v2.5 (C-5): group delay peak picking (optional).
        // Requires useERB_; only runs when useGroupDelay_ is on. Per-frame,
        // independent (no cross-frame phase smoothing):
        //   1. phase[k] = atan2(im, re) on SC complex coeffs (skip DC/Nyquist)
        //   2. unwrap across k so adjacent diff lies in (-pi, pi]
        //   3. GD[k] = -(phase_uw[k] - phase_uw[k-1]); clamp |GD| <= FFT_SIZE
        //   4. per ERB band: 75th percentile of 20*log10|GD| -> gdFloorBuf_[b] (dB)
        // The per-bin GR loop below then merges gd_prom_db with magnitude prom_dB
        // via d(k) = max(prom_mag, alphaGd_ * prom_gd).
        const bool gdActive = (useERB_ && useGroupDelay_);
        if (gdActive) {
            // 1. raw phase (DC/Nyquist excluded; stored as 0 placeholder)
            phaseBuf_[0] = 0.f;
            phaseBuf_[NUM_BINS - 1] = 0.f;
            for (int k = 1; k < NUM_BINS - 1; ++k) {
                float re = fftScFreq_[2 * k];
                float im = fftScFreq_[2 * k + 1];
                phaseBuf_[k] = std::atan2(im, re);
            }
            // 2. unwrap across bins (per-frame, fresh each call)
            const float TWO_PI = 2.f * (float)M_PI;
            for (int k = 2; k < NUM_BINS - 1; ++k) {
                float d = phaseBuf_[k] - phaseBuf_[k - 1];
                while (d > (float)M_PI)  { phaseBuf_[k] -= TWO_PI; d -= TWO_PI; }
                while (d < -(float)M_PI) { phaseBuf_[k] += TWO_PI; d += TWO_PI; }
            }
            // 3. GD via backward phase diff (normalized; keep magnitude only)
            gdBuf_[0] = 0.f;
            gdBuf_[NUM_BINS - 1] = 0.f;
            const float GD_CLAMP = (float)FFT_SIZE;
            for (int k = 1; k < NUM_BINS - 1; ++k) {
                float gd = -(phaseBuf_[k] - phaseBuf_[k - 1]);
                if (gd >  GD_CLAMP) gd =  GD_CLAMP;
                if (gd < -GD_CLAMP) gd = -GD_CLAMP;
                gdBuf_[k] = std::fabs(gd);
            }
            // 4. per ERB band 75th percentile of 20*log10|GD| (dB) into gdFloorBuf_
            //    Reuses erbScratch_ (sized NUM_BINS) — safe because the v2.3
            //    magnitude percentile pass has already consumed it for this frame.
            const int nBands = erb_.numBands();
            constexpr float GD_MIN = 1e-9f;
            for (int b = 0; b < nBands; ++b) {
                int s = erb_.bandStart(b);
                int e = erb_.bandEnd(b);
                int n = e - s;
                if (n <= 0) { gdFloorBuf_[b] = -120.f; continue; }
                float floor_dB;
                if (n < 4) {
                    float sum = 0.f;
                    for (int k = s; k < e; ++k) {
                        float v = gdBuf_[k]; if (v < GD_MIN) v = GD_MIN;
                        sum += 20.f * std::log10(v);
                    }
                    floor_dB = sum / (float)n;
                } else {
                    for (int k = s; k < e; ++k) {
                        float v = gdBuf_[k]; if (v < GD_MIN) v = GD_MIN;
                        erbScratch_[k - s] = 20.f * std::log10(v);
                    }
                    int idx = (int)std::floor(0.75f * (float)n);
                    if (idx < 0) idx = 0;
                    if (idx >= n) idx = n - 1;
                    auto first = erbScratch_.begin();
                    std::nth_element(first, first + idx, first + n);
                    floor_dB = erbScratch_[idx];
                }
                gdFloorBuf_[b] = floor_dB;
            }
        }

        // per-bin target GR (linear gain), smoothed across frames.
        // Detection: prom = how far this bin sticks out above the local log average.
        // Threshold of 3 dB filters noise; above that, scale by 3x for stronger response.
        // weightBins_ comes from per-band amount * shape (0..1).
        const float globalScale = 3.0f;       // 3x prominence -> stronger GR at low Amount
        const float promThreshold = 3.0f;     // dB; prominence below this = no GR
        const bool maskingActive = (useERB_ && useMasking_);
        for (int k = 0; k < NUM_BINS; ++k) {
            float prom_dB = logBuf_[k] - smoothedLog_[k] - promThreshold;
            // v2.4 (C-4): masking gate. Subtract per-band masking threshold
            // (dB) from the raw prominence; bins below mask are clamped to 0.
            if (maskingActive) {
                prom_dB -= maskDb_[bandOfBin_[k]];
            }
            // v2.5 (C-5): merge group delay prominence after masking.
            //   prom_gd_dB(k) = 20*log10|GD(k)| - gdFloorBuf_[band]
            //   prom_dB <- max(prom_dB, alphaGd_ * prom_gd_dB)
            // DC/Nyquist (k=0, NUM_BINS-1) carry gdBuf_=0 so prom_gd is heavily
            // negative and the max() leaves prom_dB unchanged.
            if (gdActive) {
                float gdv = gdBuf_[k];
                if (gdv < 1e-9f) gdv = 1e-9f;
                float prom_gd_dB = 20.f * std::log10(gdv) - gdFloorBuf_[bandOfBin_[k]];
                float scaled = alphaGd_ * prom_gd_dB;
                if (scaled > prom_dB) prom_dB = scaled;
            }
            if (prom_dB < 0.f) prom_dB = 0.f;
            float w = weightBins_[k];
            if (w > 1.f) w = 1.f;
            float targetGR_dB = -prom_dB * w * globalScale;
            if (targetGR_dB < -48.f) targetGR_dB = -48.f;
            float targetLin = std::pow(10.f, targetGR_dB / 20.f);

            float prev = smoothedGR_[k];
            // "attack" = gain dropping (more reduction); "release" = gain rising back to 1
            float coef = (targetLin < prev) ? attackCoef_ : releaseCoef_;
            float next = prev + (targetLin - prev) * coef;
            smoothedGR_[k] = next;

            // ---- Display values (separate from DSP state) ----
            // displayGR_: linear 0..1, 1=no GR, 0=full kill. Triggered when
            // sidechain bin sticks out > 3 dB above local average AND band Amount > 0.
            displayGR_[k] = next;

            // Normalize spectrum magnitude for display:
            //   - divide by (FFT_SIZE/2) for FFT normalization
            //   - multiply by 2 for Hann window coherent gain compensation (window sum = N/2)
            // Net factor: 1 / (FFT_SIZE/4) approx; here use 4/FFT_SIZE.
            const float normFactor = 4.f / (float)FFT_SIZE;
            float magNorm = magMain_[k] * normFactor;
            // Temporal smoothing on display magnitude to reduce flicker (~80ms at 48k/512 hop).
            const float dispCoef = 0.25f;
            displayMag_[k] = displayMag_[k] + (magNorm - displayMag_[k]) * dispCoef;
        }

        // apply GR + PostEQ to spectrum.
        // Note: preGainBins_ already factored into magnitudes for display; the
        // spectrum bins still carry the *unmodified* phase+amplitude. Multiply
        // pre*GR*post into the complex coefficients here.
        {
            float gDC = preGainBins_[0] * smoothedGR_[0] * postGainBins_[0];
            fftFreq_[0] *= gDC;
            float gNy = preGainBins_[NUM_BINS - 1] * smoothedGR_[NUM_BINS - 1] * postGainBins_[NUM_BINS - 1];
            fftFreq_[1] *= gNy;
            for (int k = 1; k < NUM_BINS - 1; ++k) {
                float g = preGainBins_[k] * smoothedGR_[k] * postGainBins_[k];
                fftFreq_[2 * k]     *= g;
                fftFreq_[2 * k + 1] *= g;
            }
        }

        rfft_.irfft(fftFreq_.data(), fftTimeOut_.data());
        rfft_.scale(fftTimeOut_.data());

        // v2.1.1: dual-window OLA — 75% overlap.
        // SYN_LEN/HOP_SIZE = 4 frames contribute to each output sample at synthesis
        // window positions {0, H, 2H, 3H}. The clear-on-read pattern preserves the
        // forward-projected tail (past readPos_) of earlier frames so they accumulate
        // with the current frame. olaGain_ = 1/mean(S(k)) for unity reconstruction.
        int olaStart = readPos_;
        const int ringSz = OUT_RING_SIZE;
        const int frameBase = FFT_SIZE - SYN_LEN;
        for (int k = 0; k < SYN_LEN; ++k) {
            int n = frameBase + k;
            int idx = (olaStart + k) % ringSz;
            outputRing_[idx] += fftTimeOut_[n] * synWindow_[n] * olaGain_;
        }
    }

    // ---- analytic EQ helpers (magnitude only, linear) ----
    static float lowShelfMag_(float f, float fc, float gainDb) {
        if (f < 1.f) f = 1.f;
        float G = std::pow(10.f, gainDb / 20.f);
        float r = (f / fc); r *= r;
        return std::sqrt((G * G + r) / (1.f + r));
    }
    static float highShelfMag_(float f, float fc, float gainDb) {
        if (f < 1.f) f = 1.f;
        float G = std::pow(10.f, gainDb / 20.f);
        float r = (fc / f); r *= r;
        return std::sqrt((G * G + r) / (1.f + r));
    }
    static float bellMag_(float f, float fc, float Q, float gainDb) {
        if (f < 1.f) f = 1.f;
        float G = std::pow(10.f, gainDb / 20.f);
        float r = (f / fc) - (fc / f);
        float shape = 1.f / (1.f + Q * Q * r * r);
        float v = 1.f + (G * G - 1.f) * shape;
        if (v < 0.f) v = 0.f;
        return std::sqrt(v);
    }
    // band shape weights (0..1)
    static float lowShelfShape_(float f, float fc) {
        if (f < 1.f) f = 1.f;
        float r = (f / fc); r *= r;
        return 1.f / std::sqrt(1.f + r);
    }
    static float highShelfShape_(float f, float fc) {
        if (f < 1.f) f = 1.f;
        float r = (fc / f); r *= r;
        return 1.f / std::sqrt(1.f + r);
    }
    static float bellShape_(float f, float fc, float Q) {
        if (f < 1.f) f = 1.f;
        float r = (f / fc) - (fc / f);
        return 1.f / (1.f + Q * Q * r * r);
    }

    void recomputeBandShape_(int b) {
        float fc = bandFreq_[b];
        for (int k = 0; k < NUM_BINS; ++k) {
            float f = binFreq_(k);
            float s = 0.f;
            switch (b) {
                case 0: s = lowShelfShape_(f, fc);   break;
                case 1: s = bellShape_(f, fc, 1.0f); break;
                case 2: s = bellShape_(f, fc, 1.0f); break;
                case 3: s = highShelfShape_(f, fc);  break;
            }
            bandShape_[b][k] = s;
        }
    }
    void recomputeWeightBins_() {
        for (int k = 0; k < NUM_BINS; ++k) {
            // use max (not sum) so total weight stays bounded by 1.0
            float w = 0.f;
            for (int b = 0; b < NUM_BANDS; ++b) {
                float v = bandShape_[b][k] * bandAmount_[b];
                if (v > w) w = v;
            }
            if (w > 1.f) w = 1.f;
            weightBins_[k] = w;
        }
    }
    void recomputePreGainBins_() {
        for (int k = 0; k < NUM_BINS; ++k) {
            float f = binFreq_(k);
            float g = 1.f;
            g *= lowShelfMag_(f,  bandFreq_[0], bandPreDb_[0]);
            g *= bellMag_(f,      bandFreq_[1], 1.0f, bandPreDb_[1]);
            g *= bellMag_(f,      bandFreq_[2], 1.0f, bandPreDb_[2]);
            g *= highShelfMag_(f, bandFreq_[3], bandPreDb_[3]);
            preGainBins_[k] = g;
        }
    }
    void recomputePostGainBins_() {
        for (int k = 0; k < NUM_BINS; ++k) {
            float f = binFreq_(k);
            float g = 1.f;
            g *= lowShelfMag_(f,  bandFreq_[0], bandPostDb_[0]);
            g *= bellMag_(f,      bandFreq_[1], 1.0f, bandPostDb_[1]);
            g *= bellMag_(f,      bandFreq_[2], 1.0f, bandPostDb_[2]);
            g *= highShelfMag_(f, bandFreq_[3], bandPostDb_[3]);
            postGainBins_[k] = g;
        }
    }
    // 2nd order Butterworth magnitude shapes (12 dB/oct).
    static float butterHpfMag_(float f, float fc) {
        if (f < 0.5f) f = 0.5f;
        float r = (f / fc); r *= r;     // (f/fc)^2
        float r4 = r * r;               // (f/fc)^4
        return std::sqrt(r4 / (1.f + r4));
    }
    static float butterLpfMag_(float f, float fc) {
        if (f < 0.5f) f = 0.5f;
        float r = (f / fc); r *= r;
        float r4 = r * r;
        return std::sqrt(1.f / (1.f + r4));
    }

    void recomputeScFilterBins_() {
        // OFF detection: HPF<=20Hz and LPF>=20kHz => bypass (gain 1.0)
        bool hpfOff = (scHpfHz_ <= 20.5f);
        bool lpfOff = (scLpfHz_ >= 19999.f);
        for (int k = 0; k < NUM_BINS; ++k) {
            float f = binFreq_(k);
            float g = 1.f;
            if (!hpfOff) g *= butterHpfMag_(f, scHpfHz_);
            if (!lpfOff) g *= butterLpfMag_(f, scLpfHz_);
            scFilterGainBins_[k] = g;
        }
    }

    void recomputeAll_() {
        for (int b = 0; b < NUM_BANDS; ++b) recomputeBandShape_(b);
        recomputeWeightBins_();
        recomputePreGainBins_();
        recomputePostGainBins_();
        recomputeScFilterBins_();
        // v2.2: keep ERB LUT in sync with current sample rate / NUM_BINS.
        erb_.rebuild(sampleRate_, NUM_BINS);
        // v2.4: keep masking LUTs + bin-to-band map in sync.
        masking_.rebuild(sampleRate_, erb_);
        recomputeBandOfBin_();
    }

    // v2.4: build static map from STFT bin index -> owning ERB band index.
    // Used by detection block when masking gate is on to look up mask threshold.
    void recomputeBandOfBin_() {
        const int nBands = erb_.numBands();
        for (int k = 0; k < NUM_BINS; ++k) bandOfBin_[k] = 0;
        for (int b = 0; b < nBands; ++b) {
            int s = erb_.bandStart(b);
            int e = erb_.bandEnd(b);
            if (s < 0) s = 0;
            if (e > NUM_BINS) e = NUM_BINS;
            for (int k = s; k < e; ++k) bandOfBin_[k] = b;
        }
    }
    void recomputeAttackRelease_() {
        float atkMs = 1.f + attackNorm_  * (500.f  - 1.f);
        float relMs = 5.f + releaseNorm_ * (2000.f - 5.f);
        float frameRate = sampleRate_ / (float)HOP_SIZE;
        float atkFrames = (atkMs * 0.001f) * frameRate;
        float relFrames = (relMs * 0.001f) * frameRate;
        if (atkFrames < 1.f) atkFrames = 1.f;
        if (relFrames < 1.f) relFrames = 1.f;
        attackCoef_  = 1.f - std::exp(-1.f / atkFrames);
        releaseCoef_ = 1.f - std::exp(-1.f / relFrames);
    }
    inline float binFreq_(int k) const {
        return (float)k * sampleRate_ / (float)FFT_SIZE;
    }

    // ---- state ----
    float sampleRate_;
    rack::dsp::RealFFT rfft_;

    float bandFreq_[NUM_BANDS];
    float bandPreDb_[NUM_BANDS];
    float bandPostDb_[NUM_BANDS];
    float bandAmount_[NUM_BANDS];

    float attackNorm_;
    float releaseNorm_;
    float attackCoef_;
    float releaseCoef_;
    float mix_;
    float makeupDb_;
    float makeupGain_;

    std::vector<float> inputRing_;
    std::vector<float> scRing_;
    std::vector<float> outputRing_;
    std::vector<float> scMonRing_;
    std::vector<float> dryDelayL_;
    std::vector<float> dryDelayR_;
    int writePos_;
    int hopCounter_;
    int readPos_;
    int dryWritePos_;

    std::vector<float> fftTimeIn_;
    std::vector<float> fftFreq_;
    std::vector<float> fftScTimeIn_;
    std::vector<float> fftScFreq_;
    std::vector<float> fftTimeOut_;
    std::vector<float> fftScTimeOut_;

    std::vector<float> magMain_;
    std::vector<float> magSc_;
    std::vector<float> logBuf_;
    std::vector<float> smoothedLog_;
    std::vector<float> smoothedGR_;
    std::vector<float> displayMag_;
    std::vector<float> displayGR_;

    std::vector<float> window_;
    std::vector<float> synWindow_;   // v2.1: synthesis window (sqrt-Hann tail)
    float olaGain_;

    std::vector<float> preGainBins_;
    std::vector<float> postGainBins_;
    std::vector<float> bandShape_[NUM_BANDS];
    std::vector<float> weightBins_;

    // SC HPF/LPF (frequency domain bin gain, applied to SC path only)
    float scHpfHz_;
    float scLpfHz_;
    std::vector<float> scFilterGainBins_;

    // v2.2: ERB band grouping scaffold (C-2). Rebuilt in setSampleRate().
    // useERB_ flag is reserved for v2.3 detection switch; legacy 4-band path
    // remains active when false (current default).
    ERBGrouping erb_;
    bool useERB_ = false;

    // v2.3 (C-3): adaptive percentile prominence state.
    // erbScratch_ is sized NUM_BINS so any ERB band slice fits without realloc.
    // floorSmoothedDb_ is the per-band 1st-order IIR-smoothed 75th percentile.
    std::vector<float> erbScratch_;
    std::vector<float> floorSmoothedDb_;
    float floorSmoothAlpha_ = 0.3f;

    // v2.4 (C-4): psychoacoustic masking gate. Default off so detection path
    // matches v2.3 bit-for-bit when not enabled.
    MaskingModel masking_;
    bool useMasking_ = false;
    std::vector<float> bandEnergyDb_;   // per-band mean log mag (dB)
    std::vector<float> maskDb_;         // per-band masking threshold (dB)
    std::vector<int>   bandOfBin_;      // STFT bin -> ERB band map

    // v2.5 (C-5): group delay peak picking. Default off; only contributes when
    // useERB_ && useGroupDelay_ both true. alphaGd_ weights gd_prom_dB against
    // magnitude prom_dB via max(); 0.5 keeps magnitude detection dominant.
    bool useGroupDelay_ = false;
    float alphaGd_ = 0.5f;
    std::vector<float> phaseBuf_;       // per-frame SC unwrapped phase workspace
    std::vector<float> gdBuf_;          // |GD(k)| (normalized, sample units)
    std::vector<float> gdFloorBuf_;     // per-ERB-band 75th pct log|GD| floor (dB)
};

// C++14 ODR definition for static constexpr array
constexpr float FFTCompDSP::DEFAULT_FREQS[4];
