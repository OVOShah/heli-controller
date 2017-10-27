#include "LowPassFilter.h"

#define M_PIf   3.14159265358979323846f

/* Approximates first order low-pass filter: H(s) = 1/(tau*s + 1)
 * See: https://en.wikipedia.org/wiki/Exponential_smoothing
 * Also read: http://techteach.no/simview/lowpass_filter/doc/filter_algorithm.pdf
 * prevOutput is the previous output from the filter
 * Input is the new input to the filter
 * Fc is the cutoff frequency in Hz
 * dt is the time between last filter time
 */
float applyLowPass(low_pass_t *low_pass, float input, float dt) {
    const float tau = 1.0f/(2.0f*M_PIf*low_pass->Fc); // Time constant
    const float alpha = dt/(tau + dt); // See (10) in http://techteach.no/simview/lowpass_filter/doc/filter_algorithm.pdf

    // y(n) = y(n-1) + alpha*(u(n) - y(n-1))
    float output = low_pass->prevOutput + alpha*(input - low_pass->prevOutput);
    low_pass->prevOutput = output;
    return output;
}
