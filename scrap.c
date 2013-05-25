// Headers needed for USB ADC/DAC
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <asm/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>

                                            sizeof(struct servo_t *));
    } else {
      // if a channel becomes live again, switch it back
      servo[i]->pid.dead = ACTIVE;
    }
    servo[i]->output = output;

    if((servo[i]->pid.active == ACTIVE) && (servo[i]->pid.dead != INACTIVE))
      printf("setting value");

  }
  return val_now;
}

        /* max_atan = 2 * param_val_curr[servo[i]->pid.sat_idx] / M_PI;
        residual = max_atan * atan((param_val_curr[servo[i]->pid.set_idx] -
                                    servo[i]->filt.val) / max_atan); */


