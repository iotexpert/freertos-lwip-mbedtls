//
// These are the functions that FreeRTOS expects the user to provide when
// using FreeRTOS.  This file also contains the implementation of malloc/calloc/free
// based on the allocator found in FreeRTOS.
//

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <string.h>
#include <cy_pdl.h>
#include <cyhal.h>
#include <cy_retarget_io.h>

void vApplicationTickHook()
{
}

void vApplicationIdleHook()
{
}

void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName )
{
    taskDISABLE_INTERRUPTS();
    for( ;; );
}

void vApplicationMallocFailedHook()
{
    taskDISABLE_INTERRUPTS();
    for( ;; );    
}

#if configUSE_RTOS_HEAP
//
// Override and use the FreeRTOS allocator
//
void *malloc(size_t amount)
{
    void *p = pvPortMalloc(amount) ;
    return p ;
}

//
// Override and use the FreeRTOS allocator
//
void *malloc_r(size_t amount)
{
    void *p = pvPortMalloc(amount) ;
    return p ;
}

//
// Override and use the FreeRTOS allocator
//
void free(void *p)
{
    vPortFree(p) ;
}

//
// Override and use the FreeRTOS allocator
//
void free_r(void *p)
{
    vPortFree(p) ;
}

//
// Override and use the FreeRTOS allocator
//
void *calloc(size_t num, size_t size)
{
    void *p = pvPortMalloc(num * size) ;
    memset(p, 0, num * size) ;
    return p ;
}

//
// Override and use the FreeRTOS allocator
//
void *calloc_r(size_t num, size_t size)
{
    void *p = pvPortMalloc(num * size) ;
    memset(p, 0, num * size) ;
    return p ;
}

//
// Override and use the FreeRTOS allocator
//
void cfree(void *p)
{
    vPortFree(p) ;
}

//
// Override and use the FreeRTOS allocator
//
void cfree_r(void *p)
{
    vPortFree(p) ;
}

//
// Override and use the FreeRTOS allocator
//
void *realloc(void *p, size_t size)
{
    void *mem = pvPortMalloc(size) ;
    memcpy(mem, p, size) ;
    vPortFree(p) ;
    return mem ;
}

//
// Override and use the FreeRTOS allocator
//
void *realloc_r(void *p, size_t size)
{
    void *mem = pvPortMalloc(size) ;
    memcpy(mem, p, size) ;
    vPortFree(p) ;
    return mem ;   
}
#endif
