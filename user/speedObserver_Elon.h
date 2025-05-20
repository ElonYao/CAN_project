#ifndef _SPEEDOB_ELON_H
#define _SPEEDOB_ELON_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <string.h>

#define MATH_2PI 6.2831853f
#define MATH_PI 3.1415926f
#define RADS2RPM 9.54929f
//Code area
typedef struct _speed_ob_
{
    float ref;
    float fbv;
    float Kp;
    float Ki;
    float Up;
    float Ui;
    float err;
    float max;
    float min;
    float out;
    float thetaStep;
    float speedHz;
}speedObserver_obj;

typedef struct _speed_ob_ *speedObserverHandle;

speedObserverHandle speedObserverInit(void *pmemory,const size_t numBytes);

void speedObserver(speedObserverHandle handle,float input);
void speedObserverReset(speedObserverHandle handle);






#ifdef __cplusplus
}
#endif


#endif 

