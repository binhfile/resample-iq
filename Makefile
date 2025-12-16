# Makefile for IQ Resampler
CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -O3 -march=native -mtune=native
LDFLAGS = -lm

# Intel IPP paths (adjust according to your installation)
IPP_ROOT ?= /opt/intel/oneapi/ipp/latest
IPP_INCLUDE = -I$(IPP_ROOT)/include
IPP_LIBS = -L$(IPP_ROOT)/lib/intel64 -lippcore -lipps -lippvm
IPP_RPATH = -Wl,-rpath,$(IPP_ROOT)/lib/intel64

# Targets
.PHONY: all clean cpp ipp

all: cpp

cpp: test_resampler_cpp

ipp: test_resampler_ipp

# Pure C++ version
test_resampler_cpp: test_resampler.cpp iq_resampler.h
	$(CXX) $(CXXFLAGS) test_resampler.cpp -o test_resampler_cpp $(LDFLAGS)

# Intel IPP version
test_resampler_ipp: test_resampler.cpp iq_resampler.h
	$(CXX) $(CXXFLAGS) -DUSE_IPP $(IPP_INCLUDE) test_resampler.cpp -o test_resampler_ipp $(IPP_LIBS) $(IPP_RPATH) $(LDFLAGS)

clean:
	rm -f test_resampler_cpp test_resampler_ipp

# Help
help:
	@echo "Available targets:"
	@echo "  make cpp    - Build pure C++ version (default)"
	@echo "  make ipp    - Build Intel IPP version"
	@echo "  make all    - Build C++ version"
	@echo "  make clean  - Remove compiled binaries"
	@echo ""
	@echo "To build IPP version with custom path:"
	@echo "  make ipp IPP_ROOT=/path/to/ipp"
