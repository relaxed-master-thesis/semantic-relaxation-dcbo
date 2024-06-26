# Data structure description

A very simple yet efficient lock-free FIFO queue. It uses a linked list (similar to the MS queue) of queue segments. Each segment contains a totally ordered bounded queue buffer, where operations are assigned to buffer cells by using FAA on enqueue and dequeue counters.

## Origin

Not published in a paper, but rather in [this 2016 blog post](http://concurrencyfreaks.blogspot.com/2016/11/faaarrayqueue-mpmc-lock-free-queue-part.html) by Pedro Ramalhete.

## Main Author

Elias Johansson
Kåre von Geijer <karev@chalmers.se>