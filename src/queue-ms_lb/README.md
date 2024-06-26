# Data structure description

The lock-based Michael-Scott queue is a two-lock version of a FIFO queue implemented as a linked list. It is similar to their lock-free FIFO queue, but slightly simpler as locking makes operations almost atomic.

## Origin

The code was introduced in the paper [Simple, fast, and practical non-blocking and blocking concurrent queue algorithms](https://doi.org/10.1145/248052.248106), and the implementation is from [https://github.com/LPD-EPFL/ASCYLIB](https://github.com/LPD-EPFL/ASCYLIB).

