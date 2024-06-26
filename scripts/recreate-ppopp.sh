#!/bin/sh

# Variables and tests for running shorter than the real paper
nbr_threads=256             # Set to the number of threads you want to use
duration=500                # Reducing more will not have that big an effect, as the setup time is not included here
relaxation_duration=100     # Computing relaxation after a run is slow, so to speed up experiments, we can reduce the time these tests run
runs=1                      # When set to 1, only runs one run for each data point in scalability experiments
step=$((nbr_threads / 4))   # Decrease this to get a more detailed plot

# Variables and used in the paper
# nbr_threads=256
# duration=500
# relaxation_duration=500
# runs=10
# step=$((nbr_threads / 8))


# d-CBO designs, normal and simple (Figure 2)
python3 scripts/benchmark.py --allow_null --plot_separate --errors timer --initial 1048576 --runs $runs --width 128 --start 2 --to $nbr_threads --step $step --include_start -d $duration --relaxation_duration $relaxation_duration --test_timeout 6000 --ndebug --prod-con dcbo-ms dcbo-faaaq dcbo-wfqueue dcbo-lcrq simple-dcbo-ms simple-dcbo-faaaq simple-dcbo-wfqueue simple-dcbo-lcrq --title "Producer-Consumer"      --name dcbo-w128-prod-con
python3 scripts/benchmark.py --allow_null --plot_separate --errors timer --initial 1048576 --runs $runs --width 128 --start 2 --to $nbr_threads --step $step --include_start -d $duration --relaxation_duration $relaxation_duration --test_timeout 6000 --ndebug            dcbo-ms dcbo-faaaq dcbo-wfqueue dcbo-lcrq simple-dcbo-ms simple-dcbo-faaaq simple-dcbo-wfqueue simple-dcbo-lcrq --title "Random Enqueue/Dequeue" --name dcbo-w128-enq-deq

# d-CBO vs d-CBL, comparing sub-queue selection (Figure 3)
python3 scripts/benchmark.py --allow_null --plot_separate --errors timer --initial 1048576 --runs $runs --width 128 --start 2 --to $nbr_threads --step $step --include_start -d $duration --relaxation_duration $relaxation_duration --test_timeout 6000 --ndebug --prod-con dcbo-ms dcbo-faaaq dcbo-wfqueue dcbo-lcrq dcbl-ms dcbl-faaaq dcbl-wfqueue dcbl-lcrq --title "Producer-Consumer"      --name dcbo-dcbl-w128-prod-con
python3 scripts/benchmark.py --allow_null --plot_separate --errors timer --initial 1048576 --runs $runs --width 128 --start 2 --to $nbr_threads --step $step --include_start -d $duration --relaxation_duration $relaxation_duration --test_timeout 6000 --ndebug            dcbo-ms dcbo-faaaq dcbo-wfqueue dcbo-lcrq dcbl-ms dcbl-faaaq dcbl-wfqueue dcbl-lcrq --title "Random Enqueue/Dequeue" --name dcbo-dcbl-w128-enq-deq

# d-CBO sub-queue scalability (Figure 4)
# TODO: How do we insert the line!?
python3 scripts/benchmark.py --allow_null --plot_separate --errors timer --initial 1048576 --runs $runs --fit_nloglogn -v w  --start 2 --to 1024 --exp_steps --start 2 -d $duration --relaxation_duration $relaxation_duration -n $nbr_threads --test_timeout 6000 --ndebug --prod-con dcbo-ms dcbo-faaaq dcbo-wfqueue dcbo-lcrq --title "Producer-Consumer"      --name dcbo-subqueue-scalability-prod-con
python3 scripts/benchmark.py --allow_null --plot_separate --errors timer --initial 1048576 --runs $runs --fit_nloglogn -v w  --start 2 --to 1024 --exp_steps --start 2 -d $duration --relaxation_duration $relaxation_duration -n $nbr_threads --test_timeout 6000 --ndebug            dcbo-ms dcbo-faaaq dcbo-wfqueue dcbo-lcrq --title "Random Enqueue/Dequeue" --name dcbo-subqueue-scalability-enq-deq

# Comparing against earlier queue implementations (Figure 5)
python3 scripts/benchmark.py --allow_null --plot_separate --errors timer --initial 1048576 --runs $runs --width 128 -l 16 --start 2 --to $nbr_threads --dcbl_dra --step $step --include_start -d $duration --relaxation_duration $relaxation_duration --test_timeout 6000 --ndebug --prod-con dcbo-faaaq simple-dcbo-faaaq dcbl-ms 2Dd-queue_optimized faaaq queue-wf-ssmem lcrq --title "Producer-Consumer"      --name state-of-the-art-comparison-prod-con-large
python3 scripts/benchmark.py --allow_null --plot_separate --errors timer --initial 1048576 --runs $runs --width 128 -l 16 --start 2 --to $nbr_threads --dcbl_dra --step $step --include_start -d $duration --relaxation_duration $relaxation_duration --test_timeout 6000 --ndebug            dcbo-faaaq simple-dcbo-faaaq dcbl-ms 2Dd-queue_optimized faaaq queue-wf-ssmem lcrq --title "Random Enqueue/Dequeue" --name state-of-the-art-comparison-enq-deq-large
python3 scripts/benchmark.py --allow_null --plot_separate --errors timer --initial 1024    --runs $runs --width 128 -l 16 --start 2 --to $nbr_threads --dcbl_dra --step $step --include_start -d $duration --relaxation_duration $relaxation_duration --test_timeout 6000 --ndebug --prod-con dcbo-faaaq simple-dcbo-faaaq dcbl-ms 2Dd-queue_optimized faaaq queue-wf-ssmem lcrq --title "Producer-Consumer"      --name state-of-the-art-comparison-prod-con-small
python3 scripts/benchmark.py --allow_null --plot_separate --errors timer --initial 1024    --runs $runs --width 128 -l 16 --start 2 --to $nbr_threads --dcbl_dra --step $step --include_start -d $duration --relaxation_duration $relaxation_duration --test_timeout 6000 --ndebug            dcbo-faaaq simple-dcbo-faaaq dcbl-ms 2Dd-queue_optimized faaaq queue-wf-ssmem lcrq --title "Random Enqueue/Dequeue" --name state-of-the-art-comparison-enq-deq-small

# BFS tests
python3 scripts/benchmark-graph.py --runs $runs -f 1 -t $nbr_threads -s $step -w 128  -l 32 --include_start --dcbl_dra --test_timeout 6000 faaaq queue-wf-ssmem lcrq 2Dd-queue_optimized dcbl-ms dcbo-faaaq dcbo-wfqueue dcbo-lcrq dcbo-ms --title "road_usa" -g "graphs/road_usa.mtx" --name bfs-road_usa-w128
python3 scripts/benchmark-graph.py --runs $runs -f 1 -t $nbr_threads -s $step -w 128  -l 32 --include_start --dcbl_dra --test_timeout 6000 faaaq queue-wf-ssmem lcrq 2Dd-queue_optimized dcbl-ms dcbo-faaaq dcbo-wfqueue dcbo-lcrq dcbo-ms --title "coPapersDBLP" -g "graphs/coPapersDBLP.mtx" --name bfs-coPapers-w128
python3 scripts/benchmark-graph.py --runs $runs -f 1 -t $nbr_threads -s $step -w 128  -l 32 --include_start --dcbl_dra --test_timeout 6000 faaaq queue-wf-ssmem lcrq 2Dd-queue_optimized dcbl-ms dcbo-faaaq dcbo-wfqueue dcbo-lcrq dcbo-ms --title "delaunay_n24" -g "graphs/delaunay_n24.mtx" --name bfs-delaunay-w128
python3 scripts/benchmark-graph.py --runs $runs -f 1 -t $nbr_threads -s $step -w 128  -l 32 --include_start --dcbl_dra --test_timeout 6000 faaaq queue-wf-ssmem lcrq 2Dd-queue_optimized dcbl-ms dcbo-faaaq dcbo-wfqueue dcbo-lcrq dcbo-ms --title "hugebubbles-00000" -g "graphs/hugebubbles-00000.mtx" --name bfs-hugebubbles-w128
python3 scripts/benchmark-graph.py --runs $runs -f 1 -t $nbr_threads -s $step -w 128  -l 32 --include_start --dcbl_dra --test_timeout 6000 faaaq queue-wf-ssmem lcrq 2Dd-queue_optimized dcbl-ms dcbo-faaaq dcbo-wfqueue dcbo-lcrq dcbo-ms --title "road_central" -g "graphs/road_central.mtx" --name bfs-road_central-w128
python3 scripts/benchmark-graph.py --runs $runs -f 1 -t $nbr_threads -s $step -w 128  -l 32 --include_start --dcbl_dra --test_timeout 6000 faaaq queue-wf-ssmem lcrq 2Dd-queue_optimized dcbl-ms dcbo-faaaq dcbo-wfqueue dcbo-lcrq dcbo-ms --title "tx2010" -g "graphs/tx2010.mtx" --name bfs-tx2010-w128
python3 scripts/benchmark-graph.py --runs $runs -f 1 -t $nbr_threads -s $step -w 128  -l 32 --include_start --dcbl_dra --test_timeout 6000 faaaq queue-wf-ssmem lcrq 2Dd-queue_optimized dcbl-ms dcbo-faaaq dcbo-wfqueue dcbo-lcrq dcbo-ms --title "asia_osm" -g "graphs/asia_osm.mtx" --name bfs-asia_osm-w128
python3 scripts/benchmark-graph.py --runs $runs -f 1 -t $nbr_threads -s $step -w 128  -l 32 --include_start --dcbl_dra --test_timeout 6000 faaaq queue-wf-ssmem lcrq 2Dd-queue_optimized dcbl-ms dcbo-faaaq dcbo-wfqueue dcbo-lcrq dcbo-ms --title "europe_osm" -g "graphs/europe_osm.mtx" --name bfs-europe_osm-w128
