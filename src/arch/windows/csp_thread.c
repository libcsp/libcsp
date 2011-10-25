#include <Windows.h>
#include <process.h>
#include "../csp_thread.h"

int csp_thread_create(csp_thread_return_t (* routine)(void *)__attribute__((stdcall)), const signed char * const thread_name, unsigned short stack_depth, void * parameters, unsigned int priority, csp_thread_handle_t * handle) {
    HANDLE taskHandle = (HANDLE) _beginthreadex(NULL, stack_depth, routine, parameters, 0, 0);
    if( taskHandle == 0 )
        return 1; // Failure
    *handle = taskHandle;
    return 0;
}
