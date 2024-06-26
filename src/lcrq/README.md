# Data structure description

A very simple yet efficient lock-free FIFO queue. It uses a linked list (similar to the MS queue) of queue segments. Each segment contains a totally ordered bounded queue buffer, where operations are assigned to buffer cells by using FAA on enqueue and dequeue counters.

## Origin

Published in the 2013 paper [Fast concurrent queues for x86 processors](https://doi.org/10.1145/2517327.2442527) by Adam Morrison and Yehuda Afek. The implementation is based on the one from [https://github.com/chaoran/fast-wait-free-queue](https://github.com/chaoran/fast-wait-free-queue).

## Main Author

Elias Johansson
Kåre von Geijer <karev@chalmers.se>