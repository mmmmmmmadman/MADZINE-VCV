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

class FFTCompDSP {
public:
    static constexpr int FFT_SIZE = 2048;
    static constexpr int HOP_SIZE = 512;
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
          makeupGain_(1.0f),
          olaGain_(1.f / 1.5f)
    {
        for (int b = 0; b < NUM_BANDS; ++b) {
            bandFreq_[b]    = DEFAULT_FREQS[b];
            bandPreDb_[b]   = 0.f;
            bandPostDb_[b]  = 0.f;
            bandAmount_[b]  = 0.f;
        }

        // ring + scratch buffers
        inputRing_.assign(FFT_SIZE, 0.f);
        scRing_.assign(FFT_SIZE, 0.f);
        outputRing_.assign(FFT_SIZE * 2, 0.f);
        scMonRing_.assign(FFT_SIZE * 2, 0.f);
        dryDelayL_.assign(FFT_SIZE, 0.f);
        dryDelayR_.assign(FFT_SIZE, 0.f);

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

        // Hann window
        window_.assign(FFT_SIZE, 0.f);
        for (int n = 0; n < FFT_SIZE; ++n) {
            window_[n] = 0.5f * (1.f - std::cos(2.f * M_PI * n / (FFT_SIZE - 1)));
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
        sampleRate_ = sr;
        recomputeAll_();
        recomputeAttackRelease_();
    }

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
        bandPreDb_[band] = dB;
        recomputePreGainBins_();
    }

    void setAmount(int band, float amt01) {
        if (band < 0 || band >= NUM_BANDS) return;
        if (amt01 < 0.f) amt01 = 0.f;
        if (amt01 > 1.f) amt01 = 1.f;
        bandAmount_[band] = amt01;
        recomputeWeightBins_();
    }

    void setAttack(float norm01) {
        if (norm01 < 0.f) norm01 = 0.f;
        if (norm01 > 1.f) norm01 = 1.f;
        attackNorm_ = norm01;
        recomputeAttackRelease_();
    }

    void setRelease(float norm01) {
        if (norm01 < 0.f) norm01 = 0.f;
        if (norm01 > 1.f) norm01 = 1.f;
        releaseNorm_ = norm01;
        recomputeAttackRelease_();
    }

    void setMix(float wet01) {
        if (wet01 < 0.f) wet01 = 0.f;
        if (wet01 > 1.f) wet01 = 1.f;
        mix_ = wet01;
    }

    void setMakeupGain(float dB) {
        if (dB < -12.f) dB = -12.f;
        if (dB >  12.f) dB =  12.f;
        makeupGain_ = std::pow(10.f, dB / 20.f);
    }

    void setScHpfFreq(float hz) {
        if (hz < 20.f) hz = 20.f;
        if (hz > 2000.f) hz = 2000.f;
        scHpfHz_ = hz;
        recomputeScFilterBins_();
    }

    void setScLpfFreq(float hz) {
        if (hz < 200.f) hz = 200.f;
        float maxHz = sampleRate_ * 0.45f;
        if (hz > 20000.f) hz = 20000.f;
        if (hz > maxHz) hz = maxHz;
        scLpfHz_ = hz;
        recomputeScFilterBins_();
    }

    void setPostEqGain(int band, float dB) {
        if (band < 0 || band >= NUM_BANDS) return;
        if (dB < -24.f) dB = -24.f;
        if (dB >  24.f) dB =  24.f;
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
        dryWritePos_ = (dryWritePos_ + 1) % FFT_SIZE;
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
        readPos_ = (readPos_ + 1) % (FFT_SIZE * 2);

        // dry aligned to FFT_SIZE latency: oldest in ring is at dryWritePos_
        float dryL = dryDelayL_[dryWritePos_];
        float dryR = dryDelayR_[dryWritePos_];

        outL = (dryL * (1.f - mix_) + wetMono * mix_) * makeupGain_;
        outR = (dryR * (1.f - mix_) + wetMono * mix_) * makeupGain_;

        // SC monitor: filtered sidechain, mono -> both channels.
        // Naturally FFT_SIZE delayed to align with main wet path.
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
        {
            int olaStart = readPos_;
            const int ringSz = FFT_SIZE * 2;
            for (int n = 0; n < FFT_SIZE; ++n) {
                int idx = (olaStart + n) % ringSz;
                scMonRing_[idx] += fftScTimeOut_[n] * window_[n] * olaGain_;
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

        // local symmetric moving average in log domain (+/- SMOOTH_N bins)
        constexpr int SMOOTH_N = 9;
        for (int k = 0; k < NUM_BINS; ++k) {
            int lo = k - SMOOTH_N; if (lo < 0) lo = 0;
            int hi = k + SMOOTH_N; if (hi > NUM_BINS - 1) hi = NUM_BINS - 1;
            float sum = 0.f;
            for (int j = lo; j <= hi; ++j) sum += logBuf_[j];
            smoothedLog_[k] = sum / (hi - lo + 1);
        }

        // per-bin target GR (linear gain), smoothed across frames.
        // Detection: prom = how far this bin sticks out above the local log average.
        // Threshold of 3 dB filters noise; above that, scale by 3x for stronger response.
        // weightBins_ comes from per-band amount * shape (0..1).
        const float globalScale = 3.0f;       // 3x prominence -> stronger GR at low Amount
        const float promThreshold = 3.0f;     // dB; prominence below this = no GR
        for (int k = 0; k < NUM_BINS; ++k) {
            float prom_dB = logBuf_[k] - smoothedLog_[k] - promThreshold;
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

        // OLA into outputRing_ starting at readPos_
        int olaStart = readPos_;
        const int ringSz = FFT_SIZE * 2;
        for (int n = 0; n < FFT_SIZE; ++n) {
            int idx = (olaStart + n) % ringSz;
            outputRing_[idx] += fftTimeOut_[n] * window_[n] * olaGain_;
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
    float olaGain_;

    std::vector<float> preGainBins_;
    std::vector<float> postGainBins_;
    std::vector<float> bandShape_[NUM_BANDS];
    std::vector<float> weightBins_;

    // SC HPF/LPF (frequency domain bin gain, applied to SC path only)
    float scHpfHz_;
    float scLpfHz_;
    std::vector<float> scFilterGainBins_;
};

// C++14 ODR definition for static constexpr array
constexpr float FFTCompDSP::DEFAULT_FREQS[4];
