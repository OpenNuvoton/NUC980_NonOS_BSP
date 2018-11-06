
#ifndef __CAP_SENSOR_H_
#define __CAP_SENSOR_H_

#include "nuc980.h"

int InitNT99141_VGA_YUV422(void);
#define NT99XXXSensorPolarity         (CAP_PAR_VSP_LOW | CAP_PAR_HSP_LOW  | CAP_PAR_PCLKP_HIGH)
#define NT99XXXDataFormatAndOrder (CAP_PAR_INDATORD_YUYV | CAP_PAR_INFMT_YUV422 | CAP_PAR_OUTFMT_YUV422)

#endif
