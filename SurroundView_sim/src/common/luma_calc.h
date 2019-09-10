#ifndef LUMA_CALC_H
#define LUMA_CALC_H

#define STEP 5

float luma_average(unsigned int width,unsigned int height,unsigned char *pixeldata);
float luma_adjust1 (float near_factor1,float near_factor2,float current_factor);

void luma_adjust2 (float *factor1,float *factor2,float *factor3,float *factor4);

//status plan1(void *buffer[4],int gain[4])

#endif