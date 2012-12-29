// The definitions of the macros in this header file changes depending on the
// use -- see servo.h.

#include "control_struct.h"
#include "servo.h"

#ifndef __NO_SERVO__
MAKE_FILTER_COEFF(boxcar_10, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1)

SERVO_INIT(srv_detector1,            // variable name
           boxcar_10,                // filter coefficients for temperature
           srv_detector1_set_idx,    // temperature setpoint
           srv_detector1_p_idx,      // P term
           srv_detector1_i_idx,      // I term
           srv_detector1_d_idx,      // D term
           srv_detector1_sat_idx,    // residual saturation (limiting) term
           srv_detector1_mem_idx)    // integral memory term

SERVO_INIT(srv_detector2,
           boxcar_10,
           srv_detector2_set_idx,
           srv_detector2_p_idx,
           srv_detector2_i_idx,
           srv_detector2_d_idx,
           srv_detector2_sat_idx,
           srv_detector2_mem_idx)

// register each servo in an enum
enum servo_sys_idx_t {
  srv_detector1_idx,
  srv_detector2_idx
};


#endif // __NO_SERVO__
