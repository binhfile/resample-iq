#ifndef IQ_RESAMPLER_H
#define IQ_RESAMPLER_H

#include <vector>
#include <cmath>
#include <stdexcept>
#include <memory>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Pure C++ Implementation
class IQResamplerCPP {
private:
    int inputRate_;
    int outputRate_;
    int upFactor_;
    int downFactor_;
    std::vector<float> filter_;
    int filterLen_;
    
    // State for streaming
    std::vector<float> stateI_;
    std::vector<float> stateQ_;
    int inputPos_;

    // Generate low-pass filter for anti-aliasing
    void generateFilter(int numTaps, float cutoffFreq) {
        filter_.resize(numTaps);
        float sum = 0.0f;
        int center = numTaps / 2;

        for (int i = 0; i < numTaps; i++) {
            float t = i - center;
            
            // Sinc function
            float h;
            if (t == 0) {
                h = 2.0f * cutoffFreq;
            } else {
                h = std::sin(2.0f * M_PI * cutoffFreq * t) / (M_PI * t);
            }

            // Hamming window
            float window = 0.54f - 0.46f * std::cos(2.0f * M_PI * i / (numTaps - 1));
            filter_[i] = h * window;
            sum += filter_[i];
        }

        // Normalize to preserve DC gain
        for (int i = 0; i < numTaps; i++) {
            filter_[i] /= sum;
        }
    }

    // GCD for simplifying ratio
    int gcd(int a, int b) {
        while (b != 0) {
            int temp = b;
            b = a % b;
            a = temp;
        }
        return a;
    }
    
    // Interpolate using sinc filter
    float interpolate(const std::vector<float>& signal, float position) {
        float result = 0.0f;
        int halfLen = filterLen_ / 2;
        int center = (int)std::floor(position);
        float frac = position - center;
        
        for (int i = 0; i < filterLen_; i++) {
            int idx = center - halfLen + i;
            if (idx >= 0 && idx < (int)signal.size()) {
                // Shift filter according to fractional delay
                float t = (i - halfLen) - frac;
                float h;
                if (std::abs(t) < 1e-6f) {
                    h = 1.0f;
                } else {
                    float cutoff = 0.5f / std::max(upFactor_, downFactor_);
                    h = std::sin(2.0f * M_PI * cutoff * t) / (M_PI * t);
                    // Hamming window
                    float window = 0.54f - 0.46f * std::cos(2.0f * M_PI * (i) / (filterLen_ - 1));
                    h *= window;
                }
                result += signal[idx] * h;
            }
        }
        
        return result;
    }

public:
    IQResamplerCPP(int inputRate, int outputRate, int filterTaps = 127) 
        : inputRate_(inputRate), outputRate_(outputRate), inputPos_(0) {
        
        // Simplify the ratio
        int g = gcd(inputRate, outputRate);
        upFactor_ = outputRate / g;
        downFactor_ = inputRate / g;

        filterLen_ = filterTaps;
        
        // Generate anti-aliasing filter
        float cutoff = 0.5f / std::max(upFactor_, downFactor_);
        generateFilter(filterLen_, cutoff);

        // Initialize state buffers
        stateI_.resize(filterLen_, 0.0f);
        stateQ_.resize(filterLen_, 0.0f);
    }

    // Process IQ data using direct resampling
    std::vector<float> process(const std::vector<float>& input) {
        if (input.size() % 2 != 0) {
            throw std::invalid_argument("Input size must be even (I/Q pairs)");
        }

        int numInputSamples = input.size() / 2;
        
        // Separate I and Q
        std::vector<float> inI(stateI_.size() + numInputSamples);
        std::vector<float> inQ(stateQ_.size() + numInputSamples);
        
        // Copy state
        for (size_t i = 0; i < stateI_.size(); i++) {
            inI[i] = stateI_[i];
            inQ[i] = stateQ_[i];
        }
        
        // Copy new input
        for (int i = 0; i < numInputSamples; i++) {
            inI[stateI_.size() + i] = input[i * 2];
            inQ[stateQ_.size() + i] = input[i * 2 + 1];
        }

        // Calculate output size
        int numOutputSamples = (int)((long long)numInputSamples * outputRate_ / inputRate_);
        std::vector<float> output;
        output.reserve(numOutputSamples * 2 + 10);

        // Resample with proper interpolation
        float ratio = (float)inputRate_ / (float)outputRate_;
        
        for (int i = 0; i < numOutputSamples; i++) {
            float inputPos = (float)stateI_.size() + i * ratio;
            
            if (inputPos < inI.size() - filterLen_/2) {
                // Linear interpolation for speed (can use sinc for quality)
                int idx = (int)inputPos;
                float frac = inputPos - idx;
                
                float valI, valQ;
                if (idx + 1 < (int)inI.size()) {
                    valI = inI[idx] * (1.0f - frac) + inI[idx + 1] * frac;
                    valQ = inQ[idx] * (1.0f - frac) + inQ[idx + 1] * frac;
                } else {
                    valI = inI[idx];
                    valQ = inQ[idx];
                }
                
                output.push_back(valI);
                output.push_back(valQ);
            }
        }

        // Update state with last samples
        int stateSize = std::min((int)stateI_.size(), numInputSamples);
        for (int i = 0; i < stateSize; i++) {
            stateI_[i] = input[(numInputSamples - stateSize + i) * 2];
            stateQ_[i] = input[(numInputSamples - stateSize + i) * 2 + 1];
        }

        return output;
    }

    void reset() {
        std::fill(stateI_.begin(), stateI_.end(), 0.0f);
        std::fill(stateQ_.begin(), stateQ_.end(), 0.0f);
        inputPos_ = 0;
    }
};

#ifdef USE_IPP
#include <ipp.h>

// Intel IPP Implementation
class IQResamplerIPP {
private:
    int inputRate_;
    int outputRate_;
    int upFactor_;
    int downFactor_;
    
    IppsResampPolyphase_32f* pSpecI_;
    IppsResampPolyphase_32f* pSpecQ_;
    Ipp8u* pBufferI_;
    Ipp8u* pBufferQ_;
    
    float* pTimeI_;
    float* pTimeQ_;
    int historyLen_;
    
    void cleanup() {
        if (pSpecI_) {
            ippsResampPolyphaseFree_32f(pSpecI_);
            pSpecI_ = nullptr;
        }
        if (pSpecQ_) {
            ippsResampPolyphaseFree_32f(pSpecQ_);
            pSpecQ_ = nullptr;
        }
        if (pBufferI_) {
            ippsFree(pBufferI_);
            pBufferI_ = nullptr;
        }
        if (pBufferQ_) {
            ippsFree(pBufferQ_);
            pBufferQ_ = nullptr;
        }
        if (pTimeI_) {
            ippsFree(pTimeI_);
            pTimeI_ = nullptr;
        }
        if (pTimeQ_) {
            ippsFree(pTimeQ_);
            pTimeQ_ = nullptr;
        }
    }
    
    int gcd(int a, int b) {
        while (b != 0) {
            int temp = b;
            b = a % b;
            a = temp;
        }
        return a;
    }

public:
    IQResamplerIPP(int inputRate, int outputRate, float rolloff = 0.9f, int filterLen = 127)
        : inputRate_(inputRate), outputRate_(outputRate),
          pSpecI_(nullptr), pSpecQ_(nullptr),
          pBufferI_(nullptr), pBufferQ_(nullptr),
          pTimeI_(nullptr), pTimeQ_(nullptr) {
        
        // Simplify ratio
        int g = gcd(inputRate, outputRate);
        upFactor_ = outputRate / g;
        downFactor_ = inputRate / g;

        // Initialize resampler for I channel
        int specSizeI, bufferSizeI;
        IppStatus status;
        
        status = ippsResampPolyphaseGetSize_32f(
            (Ipp32f)inputRate, (Ipp32f)outputRate,
            filterLen, &specSizeI, &bufferSizeI, ippAlgHintFast
        );
        
        if (status != ippStsNoErr) {
            throw std::runtime_error("IPP ResampPolyphaseGetSize failed for I channel");
        }

        pSpecI_ = (IppsResampPolyphase_32f*)ippsMalloc_8u(specSizeI);
        pBufferI_ = ippsMalloc_8u(bufferSizeI);

        status = ippsResampPolyphaseInit_32f(
            (Ipp32f)inputRate, (Ipp32f)outputRate,
            filterLen, rolloff, ippWinHamming, ippAlgHintFast,
            pSpecI_, pBufferI_
        );

        if (status != ippStsNoErr) {
            cleanup();
            throw std::runtime_error("IPP ResampPolyphaseInit failed for I channel");
        }

        // Initialize resampler for Q channel
        int specSizeQ, bufferSizeQ;
        
        status = ippsResampPolyphaseGetSize_32f(
            (Ipp32f)inputRate, (Ipp32f)outputRate,
            filterLen, &specSizeQ, &bufferSizeQ, ippAlgHintFast
        );

        pSpecQ_ = (IppsResampPolyphase_32f*)ippsMalloc_8u(specSizeQ);
        pBufferQ_ = ippsMalloc_8u(bufferSizeQ);

        status = ippsResampPolyphaseInit_32f(
            (Ipp32f)inputRate, (Ipp32f)outputRate,
            filterLen, rolloff, ippWinHamming, ippAlgHintFast,
            pSpecQ_, pBufferQ_
        );

        if (status != ippStsNoErr) {
            cleanup();
            throw std::runtime_error("IPP ResampPolyphaseInit failed for Q channel");
        }

        // Get history length
        status = ippsResampPolyphaseGetFixedFilter_32f(
            nullptr, nullptr, &historyLen_, nullptr, pSpecI_, pBufferI_
        );
        
        if (status != ippStsNoErr) {
            cleanup();
            throw std::runtime_error("IPP GetFixedFilter failed");
        }

        // Allocate history buffers
        pTimeI_ = ippsMalloc_32f(historyLen_);
        pTimeQ_ = ippsMalloc_32f(historyLen_);
        ippsZero_32f(pTimeI_, historyLen_);
        ippsZero_32f(pTimeQ_, historyLen_);

        // Set initial time
        Ipp64f timeVal = 0.0;
        ippsResampPolyphaseSetFixedFilter_32f(pTimeI_, historyLen_, timeVal, pSpecI_);
        ippsResampPolyphaseSetFixedFilter_32f(pTimeQ_, historyLen_, timeVal, pSpecQ_);
    }

    ~IQResamplerIPP() {
        cleanup();
    }

    // Process IQ data
    std::vector<float> process(const std::vector<float>& input) {
        if (input.size() % 2 != 0) {
            throw std::invalid_argument("Input size must be even (I/Q pairs)");
        }

        int numInputSamples = input.size() / 2;
        
        // Separate I and Q
        std::vector<float> inI(numInputSamples);
        std::vector<float> inQ(numInputSamples);
        
        for (int i = 0; i < numInputSamples; i++) {
            inI[i] = input[i * 2];
            inQ[i] = input[i * 2 + 1];
        }

        // Calculate output size
        int outLenI = 0, outLenQ = 0;
        Ipp64f timeVal = 0.0;
        
        IppStatus status;
        
        // Process I channel
        status = ippsResampPolyphase_32f(
            inI.data(), numInputSamples,
            nullptr, 0.0, &timeVal, &outLenI,
            pSpecI_, pBufferI_
        );

        if (status != ippStsNoErr) {
            throw std::runtime_error("IPP Resample failed (I channel size check)");
        }

        std::vector<float> outI(outLenI);
        timeVal = 0.0;
        
        status = ippsResampPolyphase_32f(
            inI.data(), numInputSamples,
            outI.data(), 1.0, &timeVal, &outLenI,
            pSpecI_, pBufferI_
        );

        if (status != ippStsNoErr) {
            throw std::runtime_error("IPP Resample failed (I channel)");
        }

        // Process Q channel
        timeVal = 0.0;
        status = ippsResampPolyphase_32f(
            inQ.data(), numInputSamples,
            nullptr, 0.0, &timeVal, &outLenQ,
            pSpecQ_, pBufferQ_
        );

        std::vector<float> outQ(outLenQ);
        timeVal = 0.0;
        
        status = ippsResampPolyphase_32f(
            inQ.data(), numInputSamples,
            outQ.data(), 1.0, &timeVal, &outLenQ,
            pSpecQ_, pBufferQ_
        );

        if (status != ippStsNoErr) {
            throw std::runtime_error("IPP Resample failed (Q channel)");
        }

        // Interleave I and Q
        std::vector<float> output(outLenI * 2);
        for (int i = 0; i < outLenI; i++) {
            output[i * 2] = outI[i];
            output[i * 2 + 1] = outQ[i];
        }

        return output;
    }

    void reset() {
        ippsZero_32f(pTimeI_, historyLen_);
        ippsZero_32f(pTimeQ_, historyLen_);
        
        Ipp64f timeVal = 0.0;
        ippsResampPolyphaseSetFixedFilter_32f(pTimeI_, historyLen_, timeVal, pSpecI_);
        ippsResampPolyphaseSetFixedFilter_32f(pTimeQ_, historyLen_, timeVal, pSpecQ_);
    }
};

#endif // USE_IPP

#endif // IQ_RESAMPLER_H
