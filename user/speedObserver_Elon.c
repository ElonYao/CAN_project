#include "speedObserver_Elon.h"

speedObserverHandle speedObserverInit(void *pmemory,const size_t numBytes)
{
    speedObserverHandle handle;
    speedObserver_obj *obj;

    if(numBytes < sizeof(speedObserver_obj))
    {
        return((speedObserverHandle)NULL);
    }

    // assign the handle
    handle = (speedObserverHandle)pmemory;
    obj= (speedObserver_obj*) handle;
    obj->thetaStep=0.002f;// observer period
    obj->max=150.0f*MATH_2PI;
    obj->min=-obj->max;
    obj->Kp=100.0f;
    obj->Ki=50.0f*0.002f;// 50*observer period
    obj->fbv=0.0f;
    obj->Ui=0.0f;
    return(handle);
}

void speedObserverRun(speedObserverHandle handle,float input)
{
    speedObserver_obj *obj=(speedObserver_obj*) handle;
    obj->ref=input;
    obj->err=obj->ref-obj->fbv;
    if(obj->err>=MATH_PI) obj->err-=MATH_2PI;
    else if(obj->err<=-MATH_PI) obj->err+=MATH_2PI;

    obj->Up=obj->err*obj->Kp;
    obj->Ui+=obj->err*obj->Ki;
    obj->Ui=(obj->Ui>obj->max)? obj->max:((obj->Ui<obj->min)? obj->min : obj->Ui);

    obj->out=obj->Up+obj->Ui;
    obj->out=(obj->out>obj->max)? obj->max:((obj->out<obj->min)? obj->min : obj->out);

    obj->speedHz=obj->out/MATH_2PI;

    // feedback signal 
    obj->fbv+=obj->out*obj->thetaStep;
    if(obj->fbv>=MATH_PI) obj->fbv-=MATH_2PI;
    else if(obj->fbv<=-MATH_PI) obj->fbv+=MATH_2PI;
}

void speedObserverReset(speedObserverHandle handle)
{
    speedObserver_obj *obj=(speedObserver_obj*) handle;
    obj->fbv=0.0f;
    obj->Ui=0.0f;
}

