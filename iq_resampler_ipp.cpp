#include "iq_resampler_ipp.h"

#ifdef USE_IPP

void IQResamplerIPP::cleanup() {
    if (pSpecI_) {
        ippsFree(pSpecI_);
        pSpecI_ = nullptr;
    }
    if (pSpecQ_) {
        ippsFree(pSpecQ_);
        pSpecQ_ = nullptr;
    }
}

int IQResamplerIPP::gcd(int a, int b) {
    while (b != 0) {
        int temp = b;
        b = a % b;
        a = temp;
    }
    return a;
}

IQResamplerIPP::IQResamplerIPP(int inputRate, int outputRate, float rolloff, int filterLen)
    : inputRate_(inputRate), outputRate_(outputRate), filterLen_(filterLen),
      pSpecI_(nullptr), pSpecQ_(nullptr) {

    // Simplify ratio
    int g = gcd(inputRate, outputRate);
    upFactor_ = outputRate / g;
    downFactor_ = inputRate / g;

    IppStatus status;
    int specSizeI, lenI, heightI;

    // Get size for I channel
    status = ippsResamplePolyphaseFixedGetSize_32f(
        inputRate, outputRate, filterLen,
        &specSizeI, &lenI, &heightI,
        ippAlgHintFast
    );

    if (status != ippStsNoErr) {
        throw std::runtime_error("IPP ResamplePolyphaseFixedGetSize failed for I channel");
    }

    // Allocate and initialize I channel
    pSpecI_ = (IppsResamplingPolyphaseFixed_32f*)ippsMalloc_8u(specSizeI);
    if (!pSpecI_) {
        throw std::runtime_error("Failed to allocate memory for I channel spec");
    }

    // Use alpha parameter for Kaiser window (typically 9.0)
    float alpha = 9.0f;
    status = ippsResamplePolyphaseFixedInit_32f(
        inputRate, outputRate, filterLen,
        rolloff, alpha,
        pSpecI_,
        ippAlgHintFast
    );

    if (status != ippStsNoErr) {
        cleanup();
        throw std::runtime_error("IPP ResamplePolyphaseFixedInit failed for I channel");
    }

    // Get size for Q channel
    int specSizeQ, lenQ, heightQ;
    status = ippsResamplePolyphaseFixedGetSize_32f(
        inputRate, outputRate, filterLen,
        &specSizeQ, &lenQ, &heightQ,
        ippAlgHintFast
    );

    if (status != ippStsNoErr) {
        cleanup();
        throw std::runtime_error("IPP ResamplePolyphaseFixedGetSize failed for Q channel");
    }

    // Allocate and initialize Q channel
    pSpecQ_ = (IppsResamplingPolyphaseFixed_32f*)ippsMalloc_8u(specSizeQ);
    if (!pSpecQ_) {
        cleanup();
        throw std::runtime_error("Failed to allocate memory for Q channel spec");
    }

    status = ippsResamplePolyphaseFixedInit_32f(
        inputRate, outputRate, filterLen,
        rolloff, alpha,
        pSpecQ_,
        ippAlgHintFast
    );

    if (status != ippStsNoErr) {
        cleanup();
        throw std::runtime_error("IPP ResamplePolyphaseFixedInit failed for Q channel");
    }
}

IQResamplerIPP::~IQResamplerIPP() {
    cleanup();
}

std::vector<float> IQResamplerIPP::process(const std::vector<float>& input) {
    if (input.size() % 2 != 0) {
        throw std::invalid_argument("Input size must be even (I/Q pairs)");
    }

    int numInputSamples = input.size() / 2;
    if (numInputSamples == 0) {
        return std::vector<float>();
    }

    // Separate I and Q
    std::vector<float> inI(numInputSamples);
    std::vector<float> inQ(numInputSamples);

    for (int i = 0; i < numInputSamples; i++) {
        inI[i] = input[i * 2];
        inQ[i] = input[i * 2 + 1];
    }

    // Calculate expected output size
    int outLen = (int)((long long)numInputSamples * outputRate_ / inputRate_) + filterLen_;
    std::vector<float> outI(outLen);
    std::vector<float> outQ(outLen);

    IppStatus status;
    int outLenI = 0, outLenQ = 0;
    Ipp64f timeI = 0.0, timeQ = 0.0;

    // Process I channel
    status = ippsResamplePolyphaseFixed_32f(
        inI.data(), numInputSamples,
        outI.data(),
        1.0f,  // norm factor
        &timeI,  // time/phase tracking
        &outLenI,
        pSpecI_
    );

    if (status != ippStsNoErr) {
        throw std::runtime_error("IPP ResamplePolyphaseFixed failed (I channel)");
    }

    // Process Q channel
    status = ippsResamplePolyphaseFixed_32f(
        inQ.data(), numInputSamples,
        outQ.data(),
        1.0f,  // norm factor
        &timeQ,  // time/phase tracking
        &outLenQ,
        pSpecQ_
    );

    if (status != ippStsNoErr) {
        throw std::runtime_error("IPP ResamplePolyphaseFixed failed (Q channel)");
    }

    // Interleave I and Q (use the minimum length)
    int actualOutLen = std::min(outLenI, outLenQ);
    std::vector<float> output(actualOutLen * 2);

    for (int i = 0; i < actualOutLen; i++) {
        output[i * 2] = outI[i];
        output[i * 2 + 1] = outQ[i];
    }

    return output;
}

void IQResamplerIPP::reset() {
    // Reinitialize the spec structures to reset state
    if (pSpecI_ && pSpecQ_) {
        float alpha = 9.0f;
        float rolloff = 0.9f;

        ippsResamplePolyphaseFixedInit_32f(
            inputRate_, outputRate_, filterLen_,
            rolloff, alpha,
            pSpecI_,
            ippAlgHintFast
        );

        ippsResamplePolyphaseFixedInit_32f(
            inputRate_, outputRate_, filterLen_,
            rolloff, alpha,
            pSpecQ_,
            ippAlgHintFast
        );
    }
}

#endif // USE_IPP
