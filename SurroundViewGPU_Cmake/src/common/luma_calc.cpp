#include "luma_calc.h"

float luma_average(unsigned int width,unsigned int height,unsigned char *pixeldata)
{
    unsigned char r = 0;
    unsigned char g = 0;
    unsigned char b = 0;
    double acumulate = 0;
    int count = 0;
    float factor =0;
    for (int i=0;i<height;i+=STEP)
    {
        for (int j=0;j<width;j+=STEP)
        {
            // if((j<=300||j>=980)&&(i<=720&&i>=360))
            if((j>200||j<1080)&&(i<=720&&i>=360))
            {
            int offset = (i*width+j)*3;
            r =*(pixeldata+offset);
            g =*(pixeldata+offset+1);
            b =*(pixeldata+offset+2);

            count++;
            acumulate += r*1.0+g*1.0+b;
            }
        }
    }
    factor = acumulate/count/3;
    return factor;
}

float luma_adjust1 (float near_factor1,float near_factor2,float current_factor)
{
    float gain =0;
    gain = (near_factor1+near_factor2)/2*current_factor;
    return gain;
}

void luma_adjust2 (float *factor1,float *factor2,float *factor3,float *factor4)
{
    float average =0;
    average = (*factor1+*factor2+*factor3+*factor4)/4;
    *factor1 = average/(*factor1);
    *factor2 = average/(*factor2);
    *factor3 = average/(*factor3);
    *factor4 = average/(*factor4);
}