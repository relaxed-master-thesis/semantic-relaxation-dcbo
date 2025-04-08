# Relaxed Semantics: d-CBO and Elastic 2D Designs

An out-of-order relaxed data structure is one where some operations deviate from the normal semantics. For example, a _k_-out-of-order FIFO queue only requires its dequeues to return one of the _k + 1_ oldest items, instead of the oldest item. The reason one might want to use relaxation is that it can lead to significantly faster data structures, especially in concurrent settings. In essence, on can trade ordering quality for performance.

This repository currently covers a majority of state-of-the-art out-of-order FIFO queues, as well as some stacks and counters, together with a selection of non-relaxed implementations to compare against. It both covers _k_-out-of-order data structures (including ones with elastic relaxation), where the 2D designs are state-of-the-art, as well as data structures with randomized out-of-order relaxation, where the _d_-CBO designs based on the choice-of-two perform the best.

This repository is originally based on the [ASCYLIB framework](https://github.com/LPD-EPFL/ASCYLIB), and memory is managed using [SSMEM](https://github.com/LPD-EPFL/ssmem), which is a simple object-based memory allocator with epoch-based garbage collection.

## Related Publications
* Balanced Allocations over Efficient Queues: A Fast Relaxed FIFO Queue
  * Kåre von Geijer, Philippas Tsigas, Elias Johansson, Sebastian Hermansson.
  * To appear in proceedings of the 30th ACM SIGPLAN Annual Symposium on Principles and Practice of Parallel Programming, PPoPP 2025.
* [How to Relax Instantly: Elastic Relaxation of Concurrent Data Structures](https://doi.org/10.1007/978-3-031-69583-4_9)
  * Kåre von Geijer, Philippas Tsigas.
  * Won the _Best Paper_ award.
  * In proceedings of the 30th International European Conference on Parallel and Distributed Computing, Euro-Par 2024.
* [Monotonically Relaxing Concurrent Data-Structure Semantics for Increasing Performance: An Efficient 2D Design Framework](https://doi.org/10.4230/LIPIcs.DISC.2019.31)
  * Adones Rukundo, Aras Atalar, Philippas Tsigas.
  * In proceedings of the 33rd International Symposium on Distributed Computing, DISC 2019.
* [Brief Announcement: 2D-Stack - A Scalable Lock-Free Stack Design that Continuously Relaxes Semantics for Better Performance](https://doi.org/10.1145/3212734.3212794)
  * Adones Rukundo, Aras Atalar, Philippas Tsigas.
  * In proceedings of the 2018 ACM Symposium on Principles of Distributed Computing, PODC 2018.

## Designs

The `src` folder includes the data structures implementations. Each of these implementations has a _README.md_ file with additional information about its origin and author.

### d-Choice Balanced Operations (d-CBO) Queues

These relaxed queues use _d_-choice load balancing to distribute operations across sub-queues in a way to achieve low relaxation errors. The _d_-CBO queues balance operation counts and are introduced in the PPoPP'25 paper _Balanced Allocations over Efficient Queues_. All _d_-CBO implementations can also be compiled to _d_-CBL that instead balance the sub-queues lenghts, as done by the _d_-RA queue from the earlier paper [Fast and Scalable, Lock-free k-FIFO Queues](https://doi.org/10.1007/978-3-642-39958-9_18). There are also _Simple d-CBO_ implementations, which use external operation counters and give up on empty-linearizability to be completely generic over sub-queue selection.
- MS d-CBO: [./src/dcbo-ms/](./src/dcbo-ms/)
- LCRQ d-CBO: [./src/dcbo-lcrq/](./src/dcbo-lcrq/)
- WFQ d-CBO: [./src/dcbo-wfqueue/](./src/dcbo-wfqueue/)
- FAAArrayQueue d-CBO: [./src/dcbo-faaaq/](./src/dcbo-faaaq/)
- MS Simple d-CBO: [./src/simple-dcbo-ms/](./src/simple-dcbo-ms/)
- LCRQ Simple d-CBO: [./src/simple-dcbo-lcrq/](./src/simple-dcbo-lcrq/)
- WFQ Simple d-CBO: [./src/simple-dcbo-wfqueue/](./src/simple-dcbo-wfqueue/)
- FAAArrayQueue Simple d-CBO: [./src/simple-dcbo-faaaq/](./src/simple-dcbo-faaaq/)

### Static 2D Designs

These designs are on a high level described in the [DISC paper](https://doi.org/10.4230/LIPIcs.DISC.2019.31), and form the foundation of the 2D framework. They have had some optimizations done in conjunction with later publications.
- 2D queue: [./src/2Dd-queue](./src/2Dd-queue)
- Optimized 2D queue: [./src/2Dd-queue_optimized](./src/2Dd-queue_optimized)
- 2Dc stack: [./src/2Dc-stack](./src/2Dc-stack)
- Optimized 2Dc stack: [./src/2Dc-stack_optimized](./src/2Dc-stack_optimized)
- 2Dd stack: [./src/2Dd-stack](./src/2Dd-stack)
- 2Dd deque: [./src/2Dd-deque](./src/2Dd-deque)
- 2Dc counter: [./src/2Dc-counter](./src/2Dc-counter)
- 2Dd counter: [./src/2Dd-counter](./src/2Dd-counter)

### Elastic 2D Designs

These designs extend the 2D stack and queue to encompass _elastic relaxation_. This means that their degree of relaxation can be changed (either manually or with a dynamic controller) during runtime. They are described in the coming Euro-Par paper.
- 2D Lateral-as-Window (LaW) queue: [./src/2Dd-queue_elastic-law](./src/2Dd-queue_elastic-law)
- 2D Lateral-plus-Window (LpW) queue: [./src/2Dd-queue_elastic-lpw](./src/2Dd-queue_elastic-lpw)
- 2D Lateral-plus-Window (LpW) stack: [./src/2Dc-stack_elastic-lpw](./src/2Dc-stack_elastic-lpw)

### Additional Relaxed Designs
These are implementations of other relaxed data structures. There are also a few additional ones in [./src/](./src/).
- k-Segment queue: [./src/queue-k-segment](./src/queue-k-segment/)
- k-Segment stack: [./src/stack-k-segment](./src/stack-k-segment/)
- d-RA queue: [./src/queue-dra](./src/queue-dra/)

### External Strict Designs

These are implementations, or copies, of external non-relaxed data structures which can be used as baselines when evaluating relaxed designs.
- Michael-Scott lock-free queue: [./src/ms](./src/ms/)
- Michael-Scott lock-based queue: [./src/queue-ms_lb](./src/queue-ms_lb/)
- LCRQ, lock-free circular buffers queue as fast as FAA: [./src/lcrq](./src/lcrq/)
- Wait-free queue as fast as FAA, using hazard pointers: [./src/queue-wf](./src/queue-wf/)
- Wait-free queue as fast as FAA, using SSMEM: [./src/queue-wf-ssmem](./src/queue-wf-ssmem/)
- FAAArrayQueue: [./src/faaaq](./src/faaaq/)
- Treiber stack: [./src/stack-treiber](./src/stack-treiber/)
- Elimination stack: [./src/stack-elimination](./src/stack-elimination/)

## Usage

Simply clone the repository and run `make` to compile all implementations, using their default tests and switches. You can then find and run the respective data structure binary benchmark in `bin/`.

The default benchmark is a synthetic test where each thread repeatedly flips a coin to either insert or remove an item. The binary takes several cli flags, which are all described by running it with `-h`, such as `make 2Dc-queue && ./bin/2Dc-queue -h`. However, the most important arguments to get started might be:
- `-d`: The duration in ms to run the experiment,
- `-i`: The number of items to insert before starting the benchmark,
- `-n`: The number of threads to use.

For the 2D data structures, you might want to start with adjusting the following parameters, which together controls its relaxation bound:
- `-l`: The depth of the 2D window,
- `-w`: The width of the 2D window.

For the d-CBO queues, you similarly adjust the width (as with most relaxed designs), and also control the sample size:
- `-w`: The number of sub-queues,
- `-c`: The _d_ in the name, specifies the number of sub-queues to sample for each operation.

### Prerequisites
The code is designed to be run on Linux and x86-64 machines, such as Intel or AMD. This is in part due to what memory ordering is assumed from the processor, and also due to the use of 128 bit compare and swaps in some data structures. Even if runnable on other architectures, some relaxation bounds will likely not hold, due to additional possible reorderings.

Furthermore, you need `gcc` and `make` to compile the tests. To run the helper scripts in [scripts/](./scripts/), you need `bc` and `python 3`. Run the following command to install the required python packages `pip3 install numpy==1.26.3 matplotlib==3.8.2 scipy==1.12.0`.

### Docker environment

There is a Dockerfile set up with the required packages for running the benchmarks, as well as plotting the results with the helper scripts. After setting up Docker, you can build the image by running
```sh
docker build -t relax-benchmarks .
```
Then you can either directly run the container to recreate the figures from the latest paper (finding the output in `results/`)
```sh
docker run --rm --hostname=example-pinning -v ./results:/app/results relax-benchmarks
```
or enter it interactively to run whatever tests you want
```sh
docker run -it --rm --hostname=example-pinning -v ./results:/app/results relax-benchmarks bash
```

Thread pinning is important for the results, and these benchmarks use the hostname to determine the order of pinned threads. Here we set the hostname to `example-pinning`, as there is a thread pinning configuration set up for that name that often performs fine. However, it is recommended to look into the last section here to set up a better configuration for thread pinning.

### Recreating paper plots

We have included scripts to re-run the experiments used in the recent publications. You might have to adjust parameters in the script, such as the number of threads, and configure thread pinning as described below. When that is done, the main difference from the plots in the papers will be cosmetic and dependent on different hardware.

These scripts are avialable in [./scripts/](./scripts/), and will output their plots and results into the ``results`` folder when done.
- Run [./scripts/recreate-ppopp.sh](./scripts/recreate-ppopp.sh) to re-run the experiments from the PPoPP 2025 paper on the _d_-CBO queue.
- Run [./scripts/recreate-europar.sh](./scripts/recreate-europar.sh) to re-run the experiments from the Euro-Par 2024 paper on elastic relaxation.

### Compilation details
Either navigate a the data structure directory and run `make`, or run `make <data structure name>` from top level, which compiles the data structure tests with the default settings. You can further set different environment variables, such as `make VERSION=O3 GC=1 INIT=one` to modify the compilation. For all possible compilation switches, see [./common/Makefile.common](./common/Makefile.common) as well as the individual Makefile for each test. Here are the most common ones:
* `VERSION` defines the optimisation level e.g. `VERSION=O3`. It takes one of the following five values:
  * `DEBUG` compile with optimisation zero and debug flags
  * `SYMBOL` compile with optimisation level three and -g
  * `O0` compile with optimisation level zero
  * `O1` compile with optimisation level one
  * `O2` compile with optimisation level two
  * `O3` compile with optimisation level three (Default)
  * `O4` compile with optimisation level three, and without any asserts
* `GC` defines if deleted nodes should be recycled `GC=1` (Default) or not `GC=0`.
* `INIT` defines if data structure initialization should be performed by all active threads `INIT=all` (Default) or one of the threads `INIT=one`
* `RELAXATION_ANALYSIS` can be set in relaxed design to measure the relaxation errors of an execution. There are two methods, and all designs don't support both.
    * `LOCK` measures the relaxation by encapsulating every linearization with a lock, exactly calculating the error at the cost of measuring an execution with essentially no parallelism. Good to validate hard upper bounds, such as for the 2D data structures.
    * `TIMER` measures the relaxation by approximately timestamping every operation. Has only a small effect on the execution profile, but cannot be used for worst-case measurements due to the approximate nature of the measurements.
      * `SAVE_TIMESTAMPS=1` can be set in order to save the timestamps of a `RELAXATION_ANALYSIS=TIMER` to save the combined get and combined put timestamps in the results/timestamps folder.
      * `SAVE_THREAD_STAMPS=1` can be set to save the thread-local timestamps
      * `SKIP_CALCULATIONS=1` can be set to not calculate the errors, best used together with `SAVE_TIMESTAMPS=1`.
* `TEST` can be used to change the benchmark used. This has been used in e.g. the d-CBO to test a BFS graph traversal, in the elastic data structures for testing dynamic scenarios. Further switches can be seen in the individual ``Makefile`` of each data structure.

### Directory description
* [src/](./src/): Contains the data structures' source code.
* [scripts/](./scripts/): Contains supporting scripts, such as ones aggregating several benchmark runs into plots.
* [results/](./results/): Default folder for test output of the scripts in [scripts](./scripts/).
* [include/](./include/): Contains support files e.g. the basic window framework ([2Dc-window.c](./include/2Dc-window.c)).
* [common/](./common/): Contains make definitions file.
* [external/](./external/): Contains external libraries such as the [ssmem](https://github.com/LPD-EPFL/ssmem) library.
* [bin/](./bin/): Contains the binary files for the compiled data structure benchmarks.

### Thread pinning
All tests will pin each software pthread to a hardware thread, using the function `pthread_setaffinity_np`. By default, the threads will be pinned sequentially to hardware threads `0,` 1, 2, 3...`. However, the numbering of the hardware threads depends on the specific CPU used, and you might often want to use a different order than the default one. For example, on a dual-socket Intel Xeon E5-2695 v4 system, the even hardware threads are on the first socket while the odd numbers are on the second, and you might not want to benchmark intra-socket behavior.

Here is a short step-by-step instruction for how to add a machine-specific pinning order:
- First, see e.g. the output from `lscpu` and `lstopo` (here we care about the `P#<...>` numbers) to understand the hardware topology.
- Then add an entry for your machine in [common/Makefile.common](./common/Makefile.common). For example, copy the one for the `athena` machine (start with `ifeq ($(PC_NAME), athena)...`), but change `athena` in the aforementioned line to the name of your computer (see output of `uname -n`), and change `ATHENA` in `-DATHENA` to a similar identifier for your computer (to be used in [include/utils.h](./include/utils.h)).
- Finally, add a matching entry to the aforementioned identifier in [include/utils.h](./include/utils.h). This entry primarily defines the order in which to pin the software threads to hardware threads. Here again, you can look at the entry for `ATHENA` for inspiration. There are three memory layouts, but the default one (the bottom-most one) is the most important to add, which should pin a thread to each core in a socket, before continuing with SMT, and finally proceeding to the next socket.
  - For a simpler example, see e.g. `ITHACA`.

Now all tests will use this pinning order. You can validate pinning orders by not allocating all hardware threads, and inspecting the output from `htop` during a test run.

