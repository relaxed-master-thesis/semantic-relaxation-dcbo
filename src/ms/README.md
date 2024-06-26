# Data structure description

The Michael-Scott queue is often seen as the first and most foundational lock-free fifo queue. It implements a linked list of one node per item, and uses compare-and-swap loops to correctly update pointers. It is a nice design, which is often used as a base-line, and whose idea is often part of many new data structure designs.

## Origin

The code was introduced in the paper [Simple, fast, and practical non-blocking and blocking concurrent queue algorithms](https://doi.org/10.1145/248052.248106).

# Main Author

Elias Johansson
Kåre von Geijer <karev@chalmers.se>

