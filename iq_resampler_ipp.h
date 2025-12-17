#ifndef IQ_RESAMPLER_IPP_H
#define IQ_RESAMPLER_IPP_H

#include <vector>
#include <stdexcept>

#ifdef USE_IPP
#include <ipp.h>

// Intel IPP Implementation
class IQResamplerIPP {
private:
    int inputRate_;
    int outputRate_;
    int upFactor_;
    int downFactor_;
    int filterLen_;

    IppsResamplingPolyphaseFixed_32f* pSpecI_;
    IppsResamplingPolyphaseFixed_32f* pSpecQ_;

    void cleanup();
    int gcd(int a, int b);

public:
    IQResamplerIPP(int inputRate, int outputRate, float rolloff = 0.9f, int filterLen = 127);
    ~IQResamplerIPP();

    // Process IQ data
    std::vector<float> process(const std::vector<float>& input);

    void reset();
};

#endif // USE_IPP

#endif // IQ_RESAMPLER_IPP_H
