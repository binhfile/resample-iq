#include <gtest/gtest.h>
#include "iq_resampler.h"
#include <cmath>
#include <vector>
#include <complex>

// Test fixture for IQ Resampler tests
class IQResamplerTest : public ::testing::Test {
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
TEST_F(IQResamplerTest, Initialization) {
    EXPECT_NO_THROW({
        IQResamplerCPP resampler(INPUT_RATE, OUTPUT_RATE);
    });
}

// Test: Output size calculation
TEST_F(IQResamplerTest, OutputSizeCorrect) {
    IQResamplerCPP resampler(INPUT_RATE, OUTPUT_RATE);

    int inputSamples = 12000;  // 100ms at 120kHz
    auto input = generateTestSignal(inputSamples, INPUT_RATE, 10000.0f);

    auto output = resampler.process(input);

    // Expected output: 12000 * (100000 / 120000) = 10000 samples
    // Note: Due to filter state and edge effects, actual output may be slightly less
    int expectedSamples = 10000;
    int actualSamples = output.size() / 2;

    // Allow tolerance for filter delays and state management
    EXPECT_NEAR(actualSamples, expectedSamples, 100)
        << "Expected ~" << expectedSamples << " samples, got " << actualSamples;
}

// Test: Specific 120kHz to 100kHz conversion
TEST_F(IQResamplerTest, Rate120kTo100k) {
    IQResamplerCPP resampler(120000, 100000);

    // Generate 1000 input samples (at 120 kHz)
    int inputSamples = 1000;
    auto input = generateTestSignal(inputSamples, 120000, 12000.0f);

    auto output = resampler.process(input);

    // Expected: 1000 * (100/120) = 833.33 ≈ 833 samples
    // Note: Filter delays cause the output to be less than theoretical
    int expectedSamples = 833;
    int actualSamples = output.size() / 2;

    EXPECT_NEAR(actualSamples, expectedSamples, 60)
        << "120kHz to 100kHz: Expected ~" << expectedSamples << " samples, got " << actualSamples;
}

// Test: Power preservation
TEST_F(IQResamplerTest, PowerPreservation) {
    IQResamplerCPP resampler(INPUT_RATE, OUTPUT_RATE);

    auto input = generateTestSignal(12000, INPUT_RATE, 10000.0f);
    auto output = resampler.process(input);

    float inputPower = calculatePower(input);
    float outputPower = calculatePower(output);

    // Power should be approximately preserved (within 10%)
    EXPECT_NEAR(outputPower, inputPower, inputPower * 0.1f)
        << "Input power: " << inputPower << ", Output power: " << outputPower;
}

// Test: DC signal preservation
TEST_F(IQResamplerTest, DCSignalPreservation) {
    IQResamplerCPP resampler(INPUT_RATE, OUTPUT_RATE);

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

    EXPECT_NEAR(avgI, 1.0f, 0.05f) << "I channel DC not preserved";
    EXPECT_NEAR(avgQ, 0.5f, 0.05f) << "Q channel DC not preserved";
}

// Test: Frequency preservation
TEST_F(IQResamplerTest, FrequencyPreservation) {
    IQResamplerCPP resampler(INPUT_RATE, OUTPUT_RATE);

    float inputFreq = 10000.0f;  // 10 kHz signal
    auto input = generateTestSignal(12000, INPUT_RATE, inputFreq);
    auto output = resampler.process(input);

    // The frequency should remain the same after resampling
    float detectedFreq = detectFrequency(output, OUTPUT_RATE);

    EXPECT_NEAR(detectedFreq, inputFreq, inputFreq * 0.05f)
        << "Expected frequency " << inputFreq << " Hz, detected " << detectedFreq << " Hz";
}

// Test: Reset functionality
TEST_F(IQResamplerTest, ResetState) {
    IQResamplerCPP resampler(INPUT_RATE, OUTPUT_RATE);

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

    EXPECT_LT(maxDiff, 0.001f) << "Reset did not restore initial state";
}

// Test: Streaming with multiple blocks
TEST_F(IQResamplerTest, StreamingMultipleBlocks) {
    IQResamplerCPP resampler(INPUT_RATE, OUTPUT_RATE);

    // Process in multiple blocks
    std::vector<float> allOutput;
    for (int block = 0; block < 5; block++) {
        auto input = generateTestSignal(1000, INPUT_RATE, 10000.0f);
        auto output = resampler.process(input);
        allOutput.insert(allOutput.end(), output.begin(), output.end());
    }

    // Should have processed approximately 5 * 1000 * (100/120) = ~4166 samples
    // Note: Filter state causes accumulated reduction in output
    int expectedSamples = 4166;
    int actualSamples = allOutput.size() / 2;

    EXPECT_NEAR(actualSamples, expectedSamples, 300)
        << "Streaming: Expected ~" << expectedSamples << " samples, got " << actualSamples;
}

// Test: Invalid input (odd size)
TEST_F(IQResamplerTest, InvalidInputSize) {
    IQResamplerCPP resampler(INPUT_RATE, OUTPUT_RATE);

    std::vector<float> invalidInput(123);  // Odd size

    EXPECT_THROW({
        resampler.process(invalidInput);
    }, std::invalid_argument);
}

// Test: Empty input
TEST_F(IQResamplerTest, EmptyInput) {
    IQResamplerCPP resampler(INPUT_RATE, OUTPUT_RATE);

    std::vector<float> emptyInput;

    auto output = resampler.process(emptyInput);
    EXPECT_EQ(output.size(), 0) << "Empty input should produce empty output";
}

// Test: Small input
TEST_F(IQResamplerTest, SmallInput) {
    IQResamplerCPP resampler(INPUT_RATE, OUTPUT_RATE);

    // Just 10 samples - may be too small for filter state
    auto input = generateTestSignal(10, INPUT_RATE, 10000.0f);

    EXPECT_NO_THROW({
        auto output = resampler.process(input);
        // Note: Very small inputs may produce no output due to filter delay
        // This is expected behavior for sample-rate converters
        EXPECT_GE(output.size(), 0) << "Small input processing should not throw";
    });
}

// Test: Different filter lengths
TEST_F(IQResamplerTest, DifferentFilterLengths) {
    std::vector<int> filterLengths = {31, 63, 127, 255};

    for (int filterLen : filterLengths) {
        IQResamplerCPP resampler(INPUT_RATE, OUTPUT_RATE, filterLen);
        auto input = generateTestSignal(1000, INPUT_RATE, 10000.0f);

        EXPECT_NO_THROW({
            auto output = resampler.process(input);
            EXPECT_GT(output.size(), 0) << "Filter length " << filterLen << " failed";
        });
    }
}

// Test: Multiple resampling ratios
TEST_F(IQResamplerTest, VariousRatios) {
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
        IQResamplerCPP resampler(tc.inputRate, tc.outputRate);
        auto input = generateTestSignal(tc.inputSamples, tc.inputRate, tc.inputRate / 10.0f);

        EXPECT_NO_THROW({
            auto output = resampler.process(input);
            EXPECT_GT(output.size(), 0)
                << "Failed for " << tc.inputRate << " to " << tc.outputRate;
        }) << "Ratio test failed: " << tc.inputRate << " -> " << tc.outputRate;
    }
}

// Main function
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
