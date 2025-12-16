# IQ Resampler: 120kHz → 100kHz

Thư viện chuyển đổi sample rate cho dữ liệu IQ từ 120kHz xuống 100kHz với hai phiên bản implementation:
- **Pure C++**: Sử dụng thuật toán polyphase filtering
- **Intel IPP**: Tận dụng thư viện Intel IPP để tăng tốc tính toán

## Đặc điểm kỹ thuật

### Tỷ lệ resampling
- Input: 120,000 Hz
- Output: 100,000 Hz  
- Ratio: 5/6 (upsampling x5, downsampling x6)

### Thuật toán
- **Polyphase filtering**: Hiệu quả cho rational resampling
- **Anti-aliasing filter**: Low-pass FIR filter với Hamming window
- **Default filter length**: 127 taps (có thể điều chỉnh)

### Dữ liệu IQ
- Format: Interleaved I/Q samples `[I0, Q0, I1, Q1, ...]`
- Type: `float` (32-bit floating point)

## Cấu trúc project

```
.
├── iq_resampler.h          # Header chứa cả 2 implementation
├── test_resampler.cpp      # Code test và benchmark
├── CMakeLists.txt          # CMake build system
├── Makefile                # Manual Makefile
└── README.md               # Tài liệu này
```

## Build Instructions

### Option 1: Sử dụng CMake (Khuyến nghị)

```bash
# Build phiên bản C++ thuần
mkdir build && cd build
cmake ..
make
./test_resampler_cpp

# Build phiên bản Intel IPP
cmake -DUSE_IPP=ON ..
make
./test_resampler_ipp
```

### Option 2: Sử dụng Makefile

```bash
# Build phiên bản C++
make cpp
./test_resampler_cpp

# Build phiên bản IPP (cần cài đặt Intel IPP)
make ipp
./test_resampler_ipp

# Custom IPP path
make ipp IPP_ROOT=/path/to/intel/ipp
```

### Option 3: Compile trực tiếp

```bash
# Pure C++
g++ -std=c++11 -O3 -march=native test_resampler.cpp -o test_resampler -lm

# Intel IPP (điều chỉnh path cho phù hợp)
g++ -std=c++11 -O3 -march=native -DUSE_IPP \
    -I/opt/intel/oneapi/ipp/latest/include \
    test_resampler.cpp \
    -L/opt/intel/oneapi/ipp/latest/lib/intel64 \
    -lippcore -lipps -lippvm -lm \
    -o test_resampler_ipp
```

## Cách sử dụng

### 1. Pure C++ Implementation

```cpp
#include "iq_resampler.h"

// Khởi tạo resampler
IQResamplerCPP resampler(120000, 100000, 127); // filter length = 127

// Chuẩn bị dữ liệu input (interleaved I/Q)
std::vector<float> input = {
    I0, Q0, I1, Q1, I2, Q2, ...
};

// Xử lý
std::vector<float> output = resampler.process(input);

// Reset state nếu cần
resampler.reset();
```

### 2. Intel IPP Implementation

```cpp
#define USE_IPP
#include "iq_resampler.h"

// Khởi tạo resampler
// rolloff = 0.9 (0.0-1.0), filterLen = 127
IQResamplerIPP resampler(120000, 100000, 0.9f, 127);

// Xử lý tương tự như C++
std::vector<float> output = resampler.process(input);
```

## API Reference

### IQResamplerCPP

#### Constructor
```cpp
IQResamplerCPP(int inputRate, int outputRate, int filterTaps = 127)
```
- `inputRate`: Sample rate đầu vào (Hz)
- `outputRate`: Sample rate đầu ra (Hz)
- `filterTaps`: Số taps của FIR filter (nên là số lẻ)

#### Methods
```cpp
std::vector<float> process(const std::vector<float>& input)
```
- Xử lý dữ liệu IQ
- Input: Vector chứa I/Q samples (kích thước phải chẵn)
- Return: Vector chứa I/Q samples đã resample

```cpp
void reset()
```
- Reset internal state của resampler

### IQResamplerIPP

#### Constructor
```cpp
IQResamplerIPP(int inputRate, int outputRate, 
               float rolloff = 0.9f, int filterLen = 127)
```
- `inputRate`: Sample rate đầu vào (Hz)
- `outputRate`: Sample rate đầu ra (Hz)
- `rolloff`: Filter rolloff factor (0.0-1.0, mặc định 0.9)
- `filterLen`: Độ dài filter (số taps)

#### Methods
Tương tự như `IQResamplerCPP`

## Performance

### Benchmarks (ước tính)

Với block size 12000 samples (100ms @ 120kHz):

| Implementation | Time (µs) | Throughput (MS/s) | Speedup |
|----------------|-----------|-------------------|---------|
| Pure C++       | ~1000-2000| ~6-12 MS/s        | 1x      |
| Intel IPP      | ~100-300  | ~40-120 MS/s      | 10-20x  |

*Note: Hiệu năng thực tế phụ thuộc vào CPU và compiler optimization*

### Optimization Tips

1. **Block Processing**: Xử lý nhiều samples cùng lúc để tận dụng cache
2. **Filter Length**: Giảm filter length để tăng tốc (trade-off với chất lượng)
3. **Compiler Flags**: Sử dụng `-O3 -march=native -mtune=native`
4. **Intel IPP**: Tối ưu hóa tốt nhất cho Intel CPUs

## Chất lượng tín hiệu

### Anti-aliasing
- Low-pass filter với cutoff = 0.45 * min(input_rate, output_rate)
- Hamming window để giảm ripple
- Trade-off giữa transition band và passband ripple

### Power Conservation
- Resampler được thiết kế để bảo toàn năng lượng tín hiệu
- Output power ≈ Input power (trong lý tưởng)
- Test code có tính toán power ratio để verify

## Dependencies

### Pure C++ version
- C++11 compiler (g++, clang++)
- Standard library (không cần thư viện ngoài)
- Math library (-lm)

### Intel IPP version  
- Intel IPP library (Integrated Performance Primitives)
- Cài đặt: https://www.intel.com/content/www/us/en/developer/tools/oneapi/ipp.html
- Components cần thiết: ippcore, ipps

## Troubleshooting

### Lỗi "Input size must be even"
- Đảm bảo số lượng samples là chẵn (vì I/Q pairs)

### IPP library not found
```bash
# Set environment variable
export IPPROOT=/opt/intel/oneapi/ipp/latest
source /opt/intel/oneapi/setvars.sh
```

### Hiệu năng không như mong đợi
- Kiểm tra compiler optimization flags
- Tăng block size khi xử lý
- Xem xét giảm filter length

## Examples

### Example 1: Basic Usage
```cpp
IQResamplerCPP resampler(120000, 100000);

// Generate test signal
std::vector<float> input(24000); // 12000 IQ samples
// ... fill input ...

auto output = resampler.process(input);
// output.size() = 20000 (10000 IQ samples)
```

### Example 2: Streaming Processing
```cpp
IQResamplerCPP resampler(120000, 100000);

while (streaming) {
    auto chunk = getNextChunk(); // Get input chunk
    auto resampled = resampler.process(chunk);
    // Process resampled data...
}

resampler.reset(); // Reset when starting new stream
```

### Example 3: Compare C++ vs IPP
```cpp
// Test data
auto testSignal = generateTestSignal(12000, 120000);

// C++ version
IQResamplerCPP cpp(120000, 100000);
auto start = std::chrono::high_resolution_clock::now();
auto output1 = cpp.process(testSignal);
auto time1 = /* calculate */;

// IPP version
#ifdef USE_IPP
IQResamplerIPP ipp(120000, 100000);
auto start = std::chrono::high_resolution_clock::now();
auto output2 = ipp.process(testSignal);
auto time2 = /* calculate */;

std::cout << "Speedup: " << (time1 / time2) << "x" << std::endl;
#endif
```

## Technical Details

### Polyphase Implementation
Thuật toán sử dụng polyphase decomposition để tối ưu hóa:
1. Upsampling by factor L (5)
2. Low-pass filtering
3. Downsampling by factor M (6)

### Filter Design
- Window: Hamming
- Cutoff: 0.45 / max(L, M)
- Normalized gain: L (để bảo toàn năng lượng)

### State Management
- Internal buffers lưu trữ filter history
- Cho phép xử lý streaming continuous data
- Reset function để bắt đầu stream mới

## License
This code is provided as-is for educational and commercial use.

## Author
Created for IQ signal processing applications requiring high-quality resampling.
