#include <stdint.h>
#ifndef CONDUCTANCE_DEMO_H
#define CONDUCTANCE_DEMO_H

void TDS_Init(void);

uint16_t TDS_GetData(void);

float TDS_GetData_PPM(void);

#endif