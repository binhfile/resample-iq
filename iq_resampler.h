#ifndef IQ_RESAMPLER_H
#define IQ_RESAMPLER_H

// Backward compatibility header
// This file includes the separate implementation files for Pure C++ and IPP

#include "iq_resampler_cpp.h"

#ifdef USE_IPP
#include "iq_resampler_ipp.h"
#endif // USE_IPP

#endif // IQ_RESAMPLER_H
