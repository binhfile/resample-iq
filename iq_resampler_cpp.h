#ifndef IQ_RESAMPLER_CPP_H
#define IQ_RESAMPLER_CPP_H

#include <vector>
#include <cmath>
#include <stdexcept>

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
    void generateFilter(int numTaps, float cutoffFreq);

    // GCD for simplifying ratio
    int gcd(int a, int b);

    // Interpolate using sinc filter
    float interpolate(const std::vector<float>& signal, float position);

public:
    IQResamplerCPP(int inputRate, int outputRate, int filterTaps = 127);

    // Process IQ data using direct resampling
    std::vector<float> process(const std::vector<float>& input);

    void reset();
};

#endif // IQ_RESAMPLER_CPP_H
