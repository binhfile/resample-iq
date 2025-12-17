#include <benchmark/benchmark.h>
#include "iq_resampler_cpp.h"

#ifdef USE_IPP
#include "iq_resampler_ipp.h"
#endif

#include <vector>
#include <cmath>
#include <random>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Generate test IQ signal with given frequency
std::vector<float> generateIQSignal(int numSamples, float sampleRate, float frequency) {
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

// Generate random IQ signal
std::vector<float> generateRandomIQSignal(int numSamples) {
    std::vector<float> signal(numSamples * 2);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(-1.0f, 1.0f);

    for (size_t i = 0; i < signal.size(); i++) {
        signal[i] = dis(gen);
    }

    return signal;
}

//==============================================================================
// Pure C++ Implementation Benchmarks
//==============================================================================

static void BM_CPP_120kTo100k_SmallBlock(benchmark::State& state) {
    IQResamplerCPP resampler(120000, 100000);
    auto input = generateIQSignal(1200, 120000, 10000);  // 10ms at 120kHz

    for (auto _ : state) {
        auto output = resampler.process(input);
        benchmark::DoNotOptimize(output);
    }

    state.SetBytesProcessed(state.iterations() * input.size() * sizeof(float));
    state.SetItemsProcessed(state.iterations() * (input.size() / 2));  // IQ samples
}
BENCHMARK(BM_CPP_120kTo100k_SmallBlock);

static void BM_CPP_120kTo100k_MediumBlock(benchmark::State& state) {
    IQResamplerCPP resampler(120000, 100000);
    auto input = generateIQSignal(12000, 120000, 10000);  // 100ms at 120kHz

    for (auto _ : state) {
        auto output = resampler.process(input);
        benchmark::DoNotOptimize(output);
    }

    state.SetBytesProcessed(state.iterations() * input.size() * sizeof(float));
    state.SetItemsProcessed(state.iterations() * (input.size() / 2));
}
BENCHMARK(BM_CPP_120kTo100k_MediumBlock);

static void BM_CPP_120kTo100k_LargeBlock(benchmark::State& state) {
    IQResamplerCPP resampler(120000, 100000);
    auto input = generateIQSignal(120000, 120000, 10000);  // 1s at 120kHz

    for (auto _ : state) {
        auto output = resampler.process(input);
        benchmark::DoNotOptimize(output);
    }

    state.SetBytesProcessed(state.iterations() * input.size() * sizeof(float));
    state.SetItemsProcessed(state.iterations() * (input.size() / 2));
}
BENCHMARK(BM_CPP_120kTo100k_LargeBlock);

static void BM_CPP_48kTo44k(benchmark::State& state) {
    IQResamplerCPP resampler(48000, 44100);
    auto input = generateIQSignal(4800, 48000, 5000);  // 100ms at 48kHz

    for (auto _ : state) {
        auto output = resampler.process(input);
        benchmark::DoNotOptimize(output);
    }

    state.SetBytesProcessed(state.iterations() * input.size() * sizeof(float));
    state.SetItemsProcessed(state.iterations() * (input.size() / 2));
}
BENCHMARK(BM_CPP_48kTo44k);

static void BM_CPP_Streaming(benchmark::State& state) {
    IQResamplerCPP resampler(120000, 100000);
    auto input = generateIQSignal(1200, 120000, 10000);

    for (auto _ : state) {
        // Simulate streaming by processing multiple small blocks
        for (int i = 0; i < 10; i++) {
            auto output = resampler.process(input);
            benchmark::DoNotOptimize(output);
        }
    }

    state.SetBytesProcessed(state.iterations() * 10 * input.size() * sizeof(float));
    state.SetItemsProcessed(state.iterations() * 10 * (input.size() / 2));
}
BENCHMARK(BM_CPP_Streaming);

static void BM_CPP_RandomSignal(benchmark::State& state) {
    IQResamplerCPP resampler(120000, 100000);
    auto input = generateRandomIQSignal(12000);

    for (auto _ : state) {
        auto output = resampler.process(input);
        benchmark::DoNotOptimize(output);
    }

    state.SetBytesProcessed(state.iterations() * input.size() * sizeof(float));
    state.SetItemsProcessed(state.iterations() * (input.size() / 2));
}
BENCHMARK(BM_CPP_RandomSignal);

//==============================================================================
// Intel IPP Implementation Benchmarks
//==============================================================================

#ifdef USE_IPP

static void BM_IPP_120kTo100k_SmallBlock(benchmark::State& state) {
    IQResamplerIPP resampler(120000, 100000);
    auto input = generateIQSignal(1200, 120000, 10000);

    for (auto _ : state) {
        auto output = resampler.process(input);
        benchmark::DoNotOptimize(output);
    }

    state.SetBytesProcessed(state.iterations() * input.size() * sizeof(float));
    state.SetItemsProcessed(state.iterations() * (input.size() / 2));
}
BENCHMARK(BM_IPP_120kTo100k_SmallBlock);

static void BM_IPP_120kTo100k_MediumBlock(benchmark::State& state) {
    IQResamplerIPP resampler(120000, 100000);
    auto input = generateIQSignal(12000, 120000, 10000);

    for (auto _ : state) {
        auto output = resampler.process(input);
        benchmark::DoNotOptimize(output);
    }

    state.SetBytesProcessed(state.iterations() * input.size() * sizeof(float));
    state.SetItemsProcessed(state.iterations() * (input.size() / 2));
}
BENCHMARK(BM_IPP_120kTo100k_MediumBlock);

static void BM_IPP_120kTo100k_LargeBlock(benchmark::State& state) {
    IQResamplerIPP resampler(120000, 100000);
    auto input = generateIQSignal(120000, 120000, 10000);

    for (auto _ : state) {
        auto output = resampler.process(input);
        benchmark::DoNotOptimize(output);
    }

    state.SetBytesProcessed(state.iterations() * input.size() * sizeof(float));
    state.SetItemsProcessed(state.iterations() * (input.size() / 2));
}
BENCHMARK(BM_IPP_120kTo100k_LargeBlock);

static void BM_IPP_48kTo44k(benchmark::State& state) {
    IQResamplerIPP resampler(48000, 44100);
    auto input = generateIQSignal(4800, 48000, 5000);

    for (auto _ : state) {
        auto output = resampler.process(input);
        benchmark::DoNotOptimize(output);
    }

    state.SetBytesProcessed(state.iterations() * input.size() * sizeof(float));
    state.SetItemsProcessed(state.iterations() * (input.size() / 2));
}
BENCHMARK(BM_IPP_48kTo44k);

static void BM_IPP_Streaming(benchmark::State& state) {
    IQResamplerIPP resampler(120000, 100000);
    auto input = generateIQSignal(1200, 120000, 10000);

    for (auto _ : state) {
        for (int i = 0; i < 10; i++) {
            auto output = resampler.process(input);
            benchmark::DoNotOptimize(output);
        }
    }

    state.SetBytesProcessed(state.iterations() * 10 * input.size() * sizeof(float));
    state.SetItemsProcessed(state.iterations() * 10 * (input.size() / 2));
}
BENCHMARK(BM_IPP_Streaming);

static void BM_IPP_RandomSignal(benchmark::State& state) {
    IQResamplerIPP resampler(120000, 100000);
    auto input = generateRandomIQSignal(12000);

    for (auto _ : state) {
        auto output = resampler.process(input);
        benchmark::DoNotOptimize(output);
    }

    state.SetBytesProcessed(state.iterations() * input.size() * sizeof(float));
    state.SetItemsProcessed(state.iterations() * (input.size() / 2));
}
BENCHMARK(BM_IPP_RandomSignal);

static void BM_IPP_DifferentRolloff(benchmark::State& state) {
    float rolloff = state.range(0) / 100.0f;
    IQResamplerIPP resampler(120000, 100000, rolloff);
    auto input = generateIQSignal(12000, 120000, 10000);

    for (auto _ : state) {
        auto output = resampler.process(input);
        benchmark::DoNotOptimize(output);
    }

    state.SetBytesProcessed(state.iterations() * input.size() * sizeof(float));
    state.SetItemsProcessed(state.iterations() * (input.size() / 2));
}
BENCHMARK(BM_IPP_DifferentRolloff)->Arg(50)->Arg(70)->Arg(90)->Arg(95);

#endif // USE_IPP

//==============================================================================
// Comparison Benchmarks (Both implementations)
//==============================================================================

static void BM_Comparison_CPP(benchmark::State& state) {
    int blockSize = state.range(0);
    IQResamplerCPP resampler(120000, 100000);
    auto input = generateIQSignal(blockSize, 120000, 10000);

    for (auto _ : state) {
        auto output = resampler.process(input);
        benchmark::DoNotOptimize(output);
    }

    state.SetBytesProcessed(state.iterations() * input.size() * sizeof(float));
    state.SetItemsProcessed(state.iterations() * (input.size() / 2));
    state.SetLabel("Pure C++");
}
BENCHMARK(BM_Comparison_CPP)
    ->Arg(1200)    // 10ms
    ->Arg(2400)    // 20ms
    ->Arg(4800)    // 40ms
    ->Arg(12000)   // 100ms
    ->Arg(24000);  // 200ms

#ifdef USE_IPP
static void BM_Comparison_IPP(benchmark::State& state) {
    int blockSize = state.range(0);
    IQResamplerIPP resampler(120000, 100000);
    auto input = generateIQSignal(blockSize, 120000, 10000);

    for (auto _ : state) {
        auto output = resampler.process(input);
        benchmark::DoNotOptimize(output);
    }

    state.SetBytesProcessed(state.iterations() * input.size() * sizeof(float));
    state.SetItemsProcessed(state.iterations() * (input.size() / 2));
    state.SetLabel("Intel IPP");
}
BENCHMARK(BM_Comparison_IPP)
    ->Arg(1200)
    ->Arg(2400)
    ->Arg(4800)
    ->Arg(12000)
    ->Arg(24000);
#endif

BENCHMARK_MAIN();
