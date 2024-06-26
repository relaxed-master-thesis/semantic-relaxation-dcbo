# Data structure description

The MS Simple d-CBO (d-Choice Balanced Operations) queue uses the choice of d to balance enqueue and dequeue counts across several sub-queues. By compiling with `HEURISTIC=LENGTH`, you instead get the d-CBL, which balances sub-queue lengths instead of operation counts. The Simple d-CBO uses external and exact counters for operation counts. The MS (Michael-Scott) queue is the most foundational lock-free queue, based on a linked list, using compare-and-swap for synchronization, and is here used as sub-queue.

## Origin

To from the paper _Balanced Allocations over Efficient Queues: A Fast Relaxed FIFO Queue_, to be published in PPoPP 2025.

## Main Author

Kåre von Geijer <karev@chalmers.se>