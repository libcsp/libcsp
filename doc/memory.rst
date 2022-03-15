How CSP uses memory
===================

CSP has been written for small microprocessor systems. The way memory is handled is therefore a tradeoff between the amount used and the code efficiency.

The current libcsp implementation primarily uses dynamic memory allocation during initialization, where all structures are allocated: port tables, connection pools, buffer pools, message queues, semaphores, tasks, etc.

Once the initiallization is complete, there are only a few functions that uses dynamic allocation, such as:

 * csp_sfp_recv() - sending larger memory chuncks than can fit into a single CSP message.
 * csp_rtable (cidr only) - adding new elements may allocate memory.

This means that there are no `alloc/free` after initialization, possibly causing fragmented memory which especially can be a problem on small systems with limited memory.
It also allows for a very simple memory allocator (implementation of `csp_malloc()`), as `free` can be avoided.

Future versions of libcsp may provide a `pure` static memory layout, since newer FreeRTOS versions allows for specifying memory for queues, semaphores, tasks, etc.
