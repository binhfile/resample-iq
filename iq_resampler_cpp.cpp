#include "iq_resampler_cpp.h"
#include <algorithm>

void IQResamplerCPP::generateFilter(int numTaps, float cutoffFreq) {
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

int IQResamplerCPP::gcd(int a, int b) {
    while (b != 0) {
        int temp = b;
        b = a % b;
        a = temp;
    }
    return a;
}

float IQResamplerCPP::interpolate(const std::vector<float>& signal, float position) {
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

IQResamplerCPP::IQResamplerCPP(int inputRate, int outputRate, int filterTaps)
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

std::vector<float> IQResamplerCPP::process(const std::vector<float>& input) {
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

void IQResamplerCPP::reset() {
    std::fill(stateI_.begin(), stateI_.end(), 0.0f);
    std::fill(stateQ_.begin(), stateQ_.end(), 0.0f);
    inputPos_ = 0;
}
