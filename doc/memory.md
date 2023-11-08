# How CSP uses memory

CSP has been written for small microprocessor systems. The way memory is
handled is therefore a tradeoff between the amount used and the code
efficiency.

All memory is statically allocated, therefore having no requirement 
to malloc in the system.

Depending on system capabilities, the number of available CSP buffers
and the maximum size of each CSP packet can be varied at compile-time.
