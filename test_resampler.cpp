#include "iq_resampler.h"
#include <iostream>
#include <chrono>
#include <cmath>
#include <iomanip>

// Generate test IQ signal
std::vector<float> generateTestSignal(int numSamples, float sampleRate) {
    std::vector<float> signal(numSamples * 2);
    
    // Generate complex sinusoid: frequency = sampleRate / 10
    float freq = sampleRate / 10.0f;
    float dt = 1.0f / sampleRate;
    
    for (int i = 0; i < numSamples; i++) {
        float t = i * dt;
        float phase = 2.0f * M_PI * freq * t;
        signal[i * 2] = std::cos(phase);      // I
        signal[i * 2 + 1] = std::sin(phase);  // Q
    }
    
    return signal;
}

// Calculate signal power
float calculatePower(const std::vector<float>& signal) {
    float power = 0.0f;
    for (size_t i = 0; i < signal.size(); i += 2) {
        float I = signal[i];
        float Q = signal[i + 1];
        power += I * I + Q * Q;
    }
    return power / (signal.size() / 2);
}

// Benchmark function
template<typename ResamplerType>
void benchmarkResampler(const std::string& name, ResamplerType& resampler, 
                       const std::vector<float>& input, int iterations = 100) {
    std::cout << "\n=== " << name << " ===" << std::endl;
    
    // Warmup
    auto output = resampler.process(input);
    
    // Benchmark
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; i++) {
        resampler.reset();
        output = resampler.process(input);
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    double avgTime = duration.count() / (double)iterations;
    
    // Calculate throughput
    int inputSamples = input.size() / 2;
    double throughput = (inputSamples / avgTime) * 1e6 / 1e6; // MSamples/sec
    
    std::cout << "Input samples:  " << inputSamples << std::endl;
    std::cout << "Output samples: " << output.size() / 2 << std::endl;
    std::cout << "Average time:   " << std::fixed << std::setprecision(2) 
              << avgTime << " µs" << std::endl;
    std::cout << "Throughput:     " << std::fixed << std::setprecision(2) 
              << throughput << " MSamples/sec" << std::endl;
    
    // Verify output
    float inputPower = calculatePower(input);
    float outputPower = calculatePower(output);
    std::cout << "Input power:    " << std::fixed << std::setprecision(6) 
              << inputPower << std::endl;
    std::cout << "Output power:   " << std::fixed << std::setprecision(6) 
              << outputPower << std::endl;
    std::cout << "Power ratio:    " << std::fixed << std::setprecision(4) 
              << (outputPower / inputPower) << std::endl;
}

int main() {
    std::cout << "IQ Resampler Test - 120kHz to 100kHz" << std::endl;
    std::cout << "====================================" << std::endl;
    
    const int INPUT_RATE = 120000;
    const int OUTPUT_RATE = 100000;
    const int NUM_SAMPLES = 12000; // 100ms of data at 120kHz
    
    // Generate test signal
    std::cout << "\nGenerating test signal..." << std::endl;
    auto testSignal = generateTestSignal(NUM_SAMPLES, INPUT_RATE);
    std::cout << "Generated " << NUM_SAMPLES << " IQ samples" << std::endl;
    
    // Test C++ implementation
    try {
        IQResamplerCPP resamplerCPP(INPUT_RATE, OUTPUT_RATE, 127);
        benchmarkResampler("Pure C++ Implementation", resamplerCPP, testSignal, 100);
    } catch (const std::exception& e) {
        std::cerr << "C++ Resampler error: " << e.what() << std::endl;
    }
    
#ifdef USE_IPP
    // Test IPP implementation
    try {
        IQResamplerIPP resamplerIPP(INPUT_RATE, OUTPUT_RATE, 0.9f, 127);
        benchmarkResampler("Intel IPP Implementation", resamplerIPP, testSignal, 100);
    } catch (const std::exception& e) {
        std::cerr << "IPP Resampler error: " << e.what() << std::endl;
    }
#else
    std::cout << "\n=== Intel IPP Implementation ===" << std::endl;
    std::cout << "Not available (compile with -DUSE_IPP and link with IPP libraries)" << std::endl;
#endif
    
    // Test with different block sizes
    std::cout << "\n\n=== Performance vs Block Size (C++) ===" << std::endl;
    std::cout << std::setw(15) << "Block Size" 
              << std::setw(15) << "Time (µs)" 
              << std::setw(20) << "Throughput (MS/s)" << std::endl;
    std::cout << std::string(50, '-') << std::endl;
    
    IQResamplerCPP resamplerCPP(INPUT_RATE, OUTPUT_RATE);
    
    for (int blockSize : {1200, 2400, 4800, 9600, 12000, 24000}) {
        auto signal = generateTestSignal(blockSize, INPUT_RATE);
        
        // Warmup
        resamplerCPP.reset();
        auto output = resamplerCPP.process(signal);
        
        // Benchmark
        auto start = std::chrono::high_resolution_clock::now();
        int iterations = std::max(10, 100000 / blockSize);
        for (int i = 0; i < iterations; i++) {
            resamplerCPP.reset();
            output = resamplerCPP.process(signal);
        }
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        double avgTime = duration.count() / (double)iterations;
        double throughput = (blockSize / avgTime) * 1e6 / 1e6;
        
        std::cout << std::setw(15) << blockSize 
                  << std::setw(15) << std::fixed << std::setprecision(2) << avgTime
                  << std::setw(20) << std::fixed << std::setprecision(2) << throughput 
                  << std::endl;
    }
    
    return 0;
}
