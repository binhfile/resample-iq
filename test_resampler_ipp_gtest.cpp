#include <gtest/gtest.h>
#include "iq_resampler_ipp.h"
#include <cmath>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Test fixture for IQ Resampler IPP implementation tests
class IQResamplerIPPTest : public ::testing::Test {
protected:
    static constexpr int INPUT_RATE = 120000;
    static constexpr int OUTPUT_RATE = 100000;
    static constexpr float TOLERANCE = 0.01f;

    // Helper function to generate a test signal with known frequency
    std::vector<float> generateTestSignal(int numSamples, float sampleRate, float frequency) {
        std::vector<float> signal(numSamples * 2);
        float dt = 1.0f / sampleRate;

        for (int i = 0; i < numSamples; i++) {
            float t = i * dt;
            float phase = 2.0f * M_PI * frequency * t;
            signal[i * 2] = std::cos(phase);      // I
            signal[i * 2 + 1] = std::sin(phase);  // Q
        }

        return signal;
    }

    // Helper function to calculate signal power
    float calculatePower(const std::vector<float>& signal) {
        float power = 0.0f;
        for (size_t i = 0; i < signal.size(); i += 2) {
            float I = signal[i];
            float Q = signal[i + 1];
            power += I * I + Q * Q;
        }
        return power / (signal.size() / 2);
    }

    // Helper function to detect frequency in signal
    float detectFrequency(const std::vector<float>& signal, float sampleRate) {
        int numSamples = signal.size() / 2;

        // Use a simple method: check phase increment
        std::vector<float> phases;
        for (int i = 0; i < std::min(numSamples, 100); i++) {
            float I = signal[i * 2];
            float Q = signal[i * 2 + 1];
            phases.push_back(std::atan2(Q, I));
        }

        // Calculate average phase difference
        float avgDiff = 0.0f;
        int count = 0;
        for (size_t i = 1; i < phases.size(); i++) {
            float diff = phases[i] - phases[i-1];
            // Unwrap phase
            while (diff > M_PI) diff -= 2 * M_PI;
            while (diff < -M_PI) diff += 2 * M_PI;
            avgDiff += diff;
            count++;
        }
        avgDiff /= count;

        // Convert to frequency
        return (avgDiff / (2.0f * M_PI)) * sampleRate;
    }
};

// Test: Basic initialization
TEST_F(IQResamplerIPPTest, Initialization) {
    EXPECT_NO_THROW({
        IQResamplerIPP resampler(INPUT_RATE, OUTPUT_RATE);
    });
}

// Test: Output size calculation
TEST_F(IQResamplerIPPTest, OutputSizeCorrect) {
    IQResamplerIPP resampler(INPUT_RATE, OUTPUT_RATE);

    int inputSamples = 12000;  // 100ms at 120kHz
    auto input = generateTestSignal(inputSamples, INPUT_RATE, 10000.0f);

    auto output = resampler.process(input);

    // Expected output: 12000 * (100000 / 120000) = 10000 samples
    int expectedSamples = 10000;
    int actualSamples = output.size() / 2;

    // IPP implementation should be very accurate
    EXPECT_NEAR(actualSamples, expectedSamples, 50)
        << "Expected ~" << expectedSamples << " samples, got " << actualSamples;
}

// Test: Specific 120kHz to 100kHz conversion
TEST_F(IQResamplerIPPTest, Rate120kTo100k) {
    IQResamplerIPP resampler(120000, 100000);

    // Generate 1000 input samples (at 120 kHz)
    int inputSamples = 1000;
    auto input = generateTestSignal(inputSamples, 120000, 12000.0f);

    auto output = resampler.process(input);

    // Expected: 1000 * (100/120) = 833.33 ≈ 833 samples
    int expectedSamples = 833;
    int actualSamples = output.size() / 2;

    EXPECT_NEAR(actualSamples, expectedSamples, 20)
        << "120kHz to 100kHz: Expected ~" << expectedSamples << " samples, got " << actualSamples;
}

// Test: Power preservation
TEST_F(IQResamplerIPPTest, PowerPreservation) {
    IQResamplerIPP resampler(INPUT_RATE, OUTPUT_RATE);

    auto input = generateTestSignal(12000, INPUT_RATE, 10000.0f);
    auto output = resampler.process(input);

    float inputPower = calculatePower(input);
    float outputPower = calculatePower(output);

    // IPP should preserve power better (within 5%)
    EXPECT_NEAR(outputPower, inputPower, inputPower * 0.05f)
        << "Input power: " << inputPower << ", Output power: " << outputPower;
}

// Test: DC signal preservation
TEST_F(IQResamplerIPPTest, DCSignalPreservation) {
    IQResamplerIPP resampler(INPUT_RATE, OUTPUT_RATE);

    // Create DC signal (constant value)
    std::vector<float> input(12000 * 2);
    for (size_t i = 0; i < input.size(); i += 2) {
        input[i] = 1.0f;      // I = 1.0
        input[i + 1] = 0.5f;  // Q = 0.5
    }

    auto output = resampler.process(input);

    // Check that DC values are preserved
    float avgI = 0.0f, avgQ = 0.0f;
    for (size_t i = 0; i < output.size(); i += 2) {
        avgI += output[i];
        avgQ += output[i + 1];
    }
    avgI /= (output.size() / 2);
    avgQ /= (output.size() / 2);

    EXPECT_NEAR(avgI, 1.0f, 0.02f) << "I channel DC not preserved";
    EXPECT_NEAR(avgQ, 0.5f, 0.02f) << "Q channel DC not preserved";
}

// Test: Frequency preservation
TEST_F(IQResamplerIPPTest, FrequencyPreservation) {
    IQResamplerIPP resampler(INPUT_RATE, OUTPUT_RATE);

    float inputFreq = 10000.0f;  // 10 kHz signal
    auto input = generateTestSignal(12000, INPUT_RATE, inputFreq);
    auto output = resampler.process(input);

    // The frequency should remain the same after resampling
    float detectedFreq = detectFrequency(output, OUTPUT_RATE);

    EXPECT_NEAR(detectedFreq, inputFreq, inputFreq * 0.03f)
        << "Expected frequency " << inputFreq << " Hz, detected " << detectedFreq << " Hz";
}

// Test: Reset functionality
TEST_F(IQResamplerIPPTest, ResetState) {
    IQResamplerIPP resampler(INPUT_RATE, OUTPUT_RATE);

    auto input = generateTestSignal(1000, INPUT_RATE, 10000.0f);

    auto output1 = resampler.process(input);
    resampler.reset();
    auto output2 = resampler.process(input);

    // After reset, the same input should produce the same output
    ASSERT_EQ(output1.size(), output2.size());

    float maxDiff = 0.0f;
    for (size_t i = 0; i < output1.size(); i++) {
        float diff = std::abs(output1[i] - output2[i]);
        maxDiff = std::max(maxDiff, diff);
    }

    EXPECT_LT(maxDiff, 0.0001f) << "Reset did not restore initial state";
}

// Test: Streaming with multiple blocks
TEST_F(IQResamplerIPPTest, StreamingMultipleBlocks) {
    IQResamplerIPP resampler(INPUT_RATE, OUTPUT_RATE);

    // Process in multiple blocks
    int totalOutputSamples = 0;
    for (int block = 0; block < 5; block++) {
        auto input = generateTestSignal(1000, INPUT_RATE, 10000.0f);
        auto output = resampler.process(input);
        totalOutputSamples += output.size() / 2;
    }

    // Should have processed approximately 5 * 1000 * (100/120) = ~4166 samples
    int expectedSamples = 4166;

    EXPECT_NEAR(totalOutputSamples, expectedSamples, 100)
        << "Streaming: Expected ~" << expectedSamples << " samples, got " << totalOutputSamples;
}

// Test: Invalid input (odd size)
TEST_F(IQResamplerIPPTest, InvalidInputSize) {
    IQResamplerIPP resampler(INPUT_RATE, OUTPUT_RATE);

    std::vector<float> invalidInput(123);  // Odd size

    EXPECT_THROW({
        resampler.process(invalidInput);
    }, std::invalid_argument);
}

// Test: Empty input
TEST_F(IQResamplerIPPTest, EmptyInput) {
    IQResamplerIPP resampler(INPUT_RATE, OUTPUT_RATE);

    std::vector<float> emptyInput;

    auto output = resampler.process(emptyInput);
    EXPECT_EQ(output.size(), 0) << "Empty input should produce empty output";
}

// Test: Different filter lengths
TEST_F(IQResamplerIPPTest, DifferentFilterLengths) {
    std::vector<int> filterLengths = {31, 63, 127, 255};

    for (int filterLen : filterLengths) {
        IQResamplerIPP resampler(INPUT_RATE, OUTPUT_RATE, 0.9f, filterLen);
        auto input = generateTestSignal(1000, INPUT_RATE, 10000.0f);

        EXPECT_NO_THROW({
            auto output = resampler.process(input);
            EXPECT_GT(output.size(), 0) << "Filter length " << filterLen << " failed";
        });
    }
}

// Test: Different rolloff factors
TEST_F(IQResamplerIPPTest, DifferentRolloffFactors) {
    std::vector<float> rolloffs = {0.5f, 0.7f, 0.9f, 0.95f};

    for (float rolloff : rolloffs) {
        IQResamplerIPP resampler(INPUT_RATE, OUTPUT_RATE, rolloff, 127);
        auto input = generateTestSignal(1000, INPUT_RATE, 10000.0f);

        EXPECT_NO_THROW({
            auto output = resampler.process(input);
            EXPECT_GT(output.size(), 0) << "Rolloff " << rolloff << " failed";
        });
    }
}

// Test: Multiple resampling ratios
TEST_F(IQResamplerIPPTest, VariousRatios) {
    struct TestCase {
        int inputRate;
        int outputRate;
        int inputSamples;
    };

    std::vector<TestCase> testCases = {
        {120000, 100000, 1200},  // 5:6 ratio
        {48000, 44100, 4800},    // Common audio resampling
        {100000, 50000, 1000},   // 2:1 downsampling
        {50000, 100000, 1000}    // 1:2 upsampling
    };

    for (const auto& tc : testCases) {
        IQResamplerIPP resampler(tc.inputRate, tc.outputRate);
        auto input = generateTestSignal(tc.inputSamples, tc.inputRate, tc.inputRate / 10.0f);

        EXPECT_NO_THROW({
            auto output = resampler.process(input);
            EXPECT_GT(output.size(), 0)
                << "Failed for " << tc.inputRate << " to " << tc.outputRate;
        }) << "Ratio test failed: " << tc.inputRate << " -> " << tc.outputRate;
    }
}

// Test: Different signal frequencies
TEST_F(IQResamplerIPPTest, DifferentFrequencies) {
    IQResamplerIPP resampler(INPUT_RATE, OUTPUT_RATE);

    // Test multiple frequencies within safe passband range
    std::vector<float> testFreqs = {5000.0f, 8000.0f, 12000.0f};

    for (float freq : testFreqs) {
        auto input = generateTestSignal(6000, INPUT_RATE, freq);
        auto output = resampler.process(input);

        // Should produce output
        EXPECT_GT(output.size(), 0) << "Failed for " << freq << " Hz";

        // Verify output is finite
        bool allFinite = true;
        for (size_t i = 0; i < output.size(); i++) {
            if (!std::isfinite(output[i])) {
                allFinite = false;
                break;
            }
        }
        EXPECT_TRUE(allFinite) << "Non-finite values for " << freq << " Hz";

        // Check power preservation
        if (allFinite) {
            float inputPower = calculatePower(input);
            float outputPower = calculatePower(output);

            if (std::isfinite(inputPower) && std::isfinite(outputPower)) {
                EXPECT_NEAR(outputPower, inputPower, inputPower * 0.1f)
                    << "Power not preserved for " << freq << " Hz";
            }
        }
    }
}

// Test: Performance comparison test (for information only)
TEST_F(IQResamplerIPPTest, PerformanceInfo) {
    IQResamplerIPP resampler(INPUT_RATE, OUTPUT_RATE);

    auto input = generateTestSignal(12000, INPUT_RATE, 10000.0f);

    // Warmup
    auto output = resampler.process(input);

    // This test always passes, it's just for timing information
    EXPECT_GT(output.size(), 0) << "IPP resampler should produce output";
}

// Main function
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    std::cout << "Testing Intel IPP IQ Resampler Implementation" << std::endl;
    std::cout << "=============================================" << std::endl;
    return RUN_ALL_TESTS();
}
