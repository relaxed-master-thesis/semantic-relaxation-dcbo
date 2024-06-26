import argparse
import numpy as np
import matplotlib.pyplot as plt
import subprocess
import re
import pickle
import json

from benchmark import RENAME_MAP, COLOR_LIST

from os import chdir, environ
from pathlib import Path
from datetime import datetime


class Bench:
    def __init__(self, structs, args, w_ratio, var, start, to, step, path, track_first, track_second, runs, show, save, ndebug, title, exp, sup_ll, sup_rl, inter_socket, hyperthreading, sup_legend, athena_points, include_start, test_timeout):
        # Yes, this should be refactored...
        self.structs = structs
        self.static_args = args
        self.width_ratio = w_ratio
        self.varying = var
        self.start = start
        self.to = to
        self.step_size = step
        self.path = path
        self.track_first = track_first
        self.track_second = track_second
        self.runs = runs
        self.test = 'BFS'
        self.show = show
        self.save = save
        self.ndebug = ndebug
        self.title = title
        self.exp_steps = exp
        self.sup_left_label = sup_ll
        self.sup_right_label = sup_rl
        self.inter_socket = inter_socket
        self.hyperthreading = hyperthreading
        self.sup_legend = sup_legend
        self.athena_points = athena_points
        self.include_start = include_start
        self.test_timeout = test_timeout

    def compile(self):
        # Compile the test for all of the structs, also check thah they exist
        my_env = environ.copy()
        my_env["TEST"] = self.test

        if self.inter_socket:
            my_env["MEMORY_SETUP"] = "numa"

        if self.hyperthreading:
            my_env["HYPERTHREAD"] = "1"

        # We might potentially want to re-instate this, measuring the relaxation in bfs and connecting it to the errors
        # if relaxation_errors == "lock":
        #     my_env["RELAXATION_ANALYSIS"] = "1"
        # elif relaxation_errors == "timer":
        #     my_env["RELAXATION_ANALYSIS"] = "TIMER"

        if self.ndebug:
            # Just 03 but with the ndebug flag
            my_env["VERSION"] = 'O4'

        for struct in self.structs:
            try:
                subprocess.check_output(['make',  f'{struct}'], env=my_env)
            except Exception as e:
                exit(e)

    def evaluate(self):
        # Runs the test for the data structures as arguments specify
        # Returns:
        #     raw_data - numpy array of all runs, dimensions: [data structure, run number, [optinally first/second track], step in variable arg, track_first/track_second]
        #     averages - numpy array of the average performance of tracked variables:
        #                    [data structure, [optionally first/second track], step in variable arg]
        #     stds - same as above, but for standard deviation
        #                    [data structure, [optionally first/second track], step in variable arg]

        # For each struct run the tests many times on each setting and get matrix, then combine those to get results
        # Dimensions: [ds, run, step, track]
        matrices = np.stack([self.run_tests(struct, self.track_first, self.track_second)
                            for struct in self.structs])

        # Transpose to match order of things in the normal benchmark.py scipt (swap step and track)
        matrices = np.transpose(matrices, (0, 1, 3, 2))

        averages = matrices.mean(axis=1)
        stds = matrices.std(axis=1)

        return matrices, averages, stds

    def run_tests(self, struct, track_first, track_second):
        # Runs all tests for a given data structure
        # Returns:
        #     Numpy mat of all test results, dimensions [varying param, track first/second]
        #         [run, step, first/second]

        results = []

        program_path = get_root_path() / 'bin' / struct
        args = self.static_args.copy()

        for _run in range(self.runs):
            run_results = []
            for var in self.var_points():
                args[f'-{self.varying}'] = var
                if self.width_ratio is not None:
                    args['-w'] = self.width_ratio * args['-n']

                arg_list = [program_path]
                for (k, v) in args.items():
                    arg_list.append(f'{k}')
                    arg_list.append(f'{v}')

                run_results.append(self.run_test(
                    arg_list, track_first, track_second))
            results.append(run_results)

        return np.array(results)

    def var_points(self):
        if self.athena_points:
            points = list(range(257))[::16]
            points[0] = 1
            return points
        elif not self.exp_steps and self.include_start:
            points = list(range(self.to, self.start-1, -self.step_size))
            if points[-1] != self.start:
                points.append(self.start)
            points.reverse()
            return points
        elif not self.exp_steps:
            return list(range(self.start, self.to + 1, self.step_size))
        else:
            acc = self.start
            points = []
            while acc <= self.to:
                points.append(acc)
                acc *= 2

            return points

    def run_test(self, args, track_first, track_second):
        # Runs a single test, returns a tuple of (track1, track2) or craches if encountering problem
        try:
            test_out = subprocess.check_output(
                args, timeout=self.test_timeout).decode('utf8')
            tracked1 = re.search(rf"{track_first} , (\d+.?\d*)", test_out)
            if track_second:
                tracked2 = re.search(rf"{track_second} , (\d+.?\d*)", test_out)
                if tracked1 and tracked2:
                    return (float(tracked1.group(1)), float(tracked2.group(1)))
                else:
                    print(f"could not parse {track_first} and {track_second} in \n{test_out} when running {args}")
                    exit(0)
            elif tracked1:
                print("Warning, only tracking one variable for bfs script is volatile")
                return float(tracked1, None)
            else:
                print(f"could not parse {track_first} in \n{test_out}")
                exit(0)

        except Exception as e:
            exit(e)

    def save_data(self, raw_data):
        if self.save:
            self.path.mkdir(parents=True)
            np.save(self.path / 'raw_data', raw_data, allow_pickle=False)

            with open(self.path / 'bench', 'wb') as f:
                pickle.dump(self, f, -1)

            with open(self.path / 'bench_dict.txt', 'w') as f:
                f.write(json.dumps({k: str(v)
                        for (k, v) in self.__dict__.items()}, indent=2))

    def plot(self, averages, stds):
        PLOT_RELATIVE = True and averages.ndim == 3

        MARKERS = ['o', '^', 'v', 'x', '*', 'o', '^', 'v', 'x', '*', 'x']

        if PLOT_RELATIVE:
            # Plot work and time as speedup and work increase, relative LCRQ with n=1
            # TODO: This should be changed to something more relative
            filtered_structs = [(index, string) for index, string in enumerate(self.structs) if 'lcrq' in string and 'dcb' not in string]
            if not filtered_structs:
                lcrq_ind = 0
                print("Warning, could not find baseline LCRQ. LCRQ is normally used to normalize the y axes. Now the first queue will be used for normalization instead.")
            else:
                lcrq_ind = min(filtered_structs, key=lambda x: len(x[1]))[0]

            # Time component
            lcrq_time = averages[lcrq_ind,0,0]
            for ds in range(averages.shape[0]):
                for conf in range(averages.shape[2]):
                    stds[ds,0,conf] = stds[ds,0,conf] / averages[ds,0,conf] * lcrq_time/averages[ds,0,conf]
                    averages[ds,0,conf] = lcrq_time/averages[ds,0,conf]

            # Work component
            base_work = np.min(averages[:,1,:])
            for ds in range(averages.shape[0]):
                for conf in range(averages.shape[2]):
                    stds[ds,1,conf] = stds[ds,1,conf] /base_work
                    averages[ds,1,conf] = averages[ds,1,conf]/base_work


        # Adjusted to fit article dimensions
        width = 350
        height = width*0.7

        # Create a new figure with the specified dimensions
        fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(
            width / 100, 1.5 * height / 100), dpi=100, sharex=True)
        x_ax = ax2

        # Title
        # plt.suptitle(self.title, fontsize=12)
        ax1.set_title(self.title, fontsize=14)

        # Automatically include some x-ticks
        xticks = self.var_points()
        last = xticks[-1]
        xticks = xticks[:-1]
        while len(xticks) > 5:
            xticks = xticks[::2]
        xticks.append(last)
        plt.xticks(xticks)

        # Find x-label
        if self.varying == 'n':
            x_ax.set_xlabel("Threads", fontsize=12)
            if self.to == 256:
                x_ax.set_xticks([1, 64, 128, 192, 256])
        elif self.varying == 'w':
            x_ax.set_xlabel("Width")
        elif self.varying == 'l':
            x_ax.set_xlabel("Depth")
        elif self.varying == 'k':
            # Rank error bound?
            x_ax.set_xlabel("Rank Error Bound", fontsize=12)
        else:
            x_ax.set_xlabel(f"{self.varying}")

        # Y-label
        if not self.sup_left_label:
            if PLOT_RELATIVE and self.track_first == 'elapsed_time':
                ax1.set_ylabel("Speedup", fontsize=10)
            elif self.track_first == 'elapsed_time':
                ax1.set_ylabel("Elapsed Time (ms)", fontsize=9)
            else:
                ax1.set_ylabel(f"{self.track_first}", fontsize=10)

            if PLOT_RELATIVE and self.track_second == "total_work":
                ax2.set_ylabel("Work Increase", fontsize=10)
            elif self.track_second == "total_work":
                ax2.set_ylabel("Total Work", fontsize=9)
            else:
                ax2.set_ylabel(f"{self.track_second}", fontsize=9)

        if self.exp_steps:
            x_ax.set_xscale("log")
            ax2.set_yscale("log")
        ax1.set_yscale("log")
        ax1.autoscale(enable=True, axis='y') # Makes the scaling more sane

        for (row, struct) in enumerate(self.structs):
            color = COLOR_LIST[row]
            # Plot Throughput
            line = ax1.errorbar(self.var_points(
            ), averages[row, 0, :], stds[row, 0, :], linestyle='-', color=color, alpha=0.85, marker=MARKERS[row % len(MARKERS)])
            line.set_label(RENAME_MAP.get(struct, struct))

            # Plot extra work
            line = ax2.errorbar(self.var_points(
            ), averages[row, 1, :], stds[row, 1, :], linestyle='-', color=color, alpha=0.85, marker=MARKERS[row % len(MARKERS)])

        handles, labels = ax1.get_legend_handles_labels()

        # Make the layout tight and nice before adding the legend outside of the figure
        # plt.tight_layout(pad=0.1, rect=[0, 0, 1, 0.95])
        plt.tight_layout()

        if not self.sup_legend:
            # fig.legend(handles, labels, loc='lower center',
            #            bbox_to_anchor=(0.5, -0.00), ncol=2, fontsize=6)
            fig.legend(handles, labels, loc='lower center',
                       bbox_to_anchor=(0.5, -0.00), ncol=3, fontsize=6)

            # Adjust the bottom parameter as needed to fit the legend
            plt.subplots_adjust(bottom=0.32)

        if self.save:
            self.path.mkdir(parents=True, exist_ok=True)
            plt.savefig(self.path / f'{self.path.name}.pdf', format='pdf')

        if self.show:
            plt.show()


def get_root_path():
    return Path(__file__).parent.parent


def load_old_bench(old_bench_path, sup_ll, sup_rl, sup_legend, title):
    with open(old_bench_path / "bench", 'rb') as f:
        old_bench = pickle.load(f)

    old_bench.sup_left_label = sup_ll
    old_bench.sup_right_label = sup_rl

    old_bench.sup_legend = sup_legend
    if title:
        old_bench.title = title

    if 'athena_points' not in old_bench.__dict__:
        old_bench.athena_points = False

    old_bench.save = True
    old_bench.show = True
    old_bench.path = Path(
        f"{old_bench_path}-replot-{datetime.now().strftime('%H:%M:%S')}")
    return old_bench


def main(args):  # test_bench, old_bench_path=None):
    chdir(get_root_path())
    # If old_bench_path is specified, load old bench instead of compiling and evaluating a new one
    if args.old_bench is not None:
        test_bench = load_old_bench(
            args.old_bench, args.sup_left_label, args.sup_right_label, args.sup_legend, args.title)
        raw_data = np.load(args.old_bench / 'raw_data.npy')
        averages = raw_data.mean(axis=1)
        stds = raw_data.std(axis=1)
        if raw_data.ndim != 4:
            print('warning not tracking 2 things')
    else:
        # Can be used from outside as well to reload a test bench
        test_bench = new_bench(args)
        test_bench.compile()
        raw_data, averages, stds = test_bench.evaluate()
        # Conditionally saves and shows data based on arguments
        test_bench.save_data(raw_data)
    test_bench.plot(averages, stds)


def new_bench(args):
    if args.width is not None:
        args.width_ratio = None

    static_args = {
        '-f': args.graph_path,
        '-w': args.width,
        '-l': args.depth,
        '-k': args.relaxation,
        '-n': args.threads,
        '-m': args.mode,
        '-c': args.choice,
        '-r': args.root,
    }

    for (key, value) in static_args.copy().items():
        if value is None:
            del static_args[key]

    datestr = datetime.now().strftime("%Y-%m-%d")
    path = get_root_path() / 'results'
    if args.name is None:
        path = path / f'{datestr}_bfs-{",".join(args.structs)}'
    else:
        path = path / f'{datestr}_{args.name}'

    if path.exists() and not args.nosave:
        timestr = datetime.now().strftime('%H:%M:%S')
        path = path.parent / f"{path.name}_{timestr}"
        print(f'Path exists, so specifying datetime: {path}')

    bench = Bench(args.structs, static_args, args.width_ratio, args.varying, args.start,
                  args.to, args.step_size, path, args.track_first, args.track_second, args.runs, args.show,
                  not args.nosave, args.ndebug, args.title, args.exp_steps,
                  args.sup_left_label, args.sup_right_label, args.inter_socket, args.hyperthreading,
                  args.sup_legend, args.athena_points, args.include_start, args.test_timeout)

    return bench


def parse_args():
    # This became really ugly. Contemplating just rewriting in Rust to use the wonderful Clap crate :)
    parser = argparse.ArgumentParser(
        description='Benchmark several tests against each other.')
    parser.add_argument('structs', nargs='*',
                        help='Adds a data structure to compare with')
    parser.add_argument('--name',
                        help='The folder within results to save the data in')
    parser.add_argument('--track_first', default='elapsed_time',
                        help='Which main metric to track from the raw outputs')
    parser.add_argument('--track_second', default='total_work',
                        help='Which secondary metric to track from the raw outputs')
    parser.add_argument('--runs', default=1, type=int,
                        help='How many runs to take the average of')
    parser.add_argument('--show', action='store_true',  # type=bool,
                        help='Flag to show the generated graph')
    parser.add_argument('--nosave', action='store_true',  # type=bool,
                        help='Flag to not save the results')
    parser.add_argument('--ndebug', action='store_true',  # type=bool,
                        help='Flag to turn off asserts')
    parser.add_argument('--varying', '-v', default='n',
                        help='What argument to vary between runs, overwrites set values')
    parser.add_argument('--start', '-f', default=1, type=int,
                        help='What to start the variable at')
    parser.add_argument('--to', '-t', default=8, type=int,
                        help='The max of the variable')
    parser.add_argument('--step_size', '-s', default=1, type=int,
                        help='How large linear steps to take, overwritten by --exp-steps')
    parser.add_argument('--exp_steps', action='store_true',
                        help='Take steps by doubling each time')

    # Now add the arguments passed into the actual program
    parser.add_argument('--width-ratio', type=int,
                        help='Sets the width as a ratio of number of threads, overwritten by --width')
    parser.add_argument('--width', '-w', type=int,
                        help='Fixed width for all runs')
    parser.add_argument('--depth', '-l', type=int,
                        help='Fixed depth for all runs')
    parser.add_argument('--relaxation', '-k', type=int,
                        help='Fixed upper relaxation bound. If width specified depth is first adjusted.')
    parser.add_argument('--mode', '-m', type=int,
                        help='Which relaxation mode to use.')
    parser.add_argument('--threads', '-n', type=int,
                        help='Fixed number of threads')
    parser.add_argument('--choice', '-c', type=int,
                        help='How many partial queues to sample in c-choice load balancers')
    parser.add_argument('--graph_path', '-g', type=str,
                        help='The path to the mtx graph file to use')
    parser.add_argument('--root', type=int,
                        help='The root to use for all experiments')

    # Some more style arguments
    parser.add_argument('--title',
                        help='What title to hav for the plot')
    parser.add_argument('--old_bench', type=Path,  default=None,
                        help='Replots the figure of a specific path')
    parser.add_argument('--sup_left_label', action='store_true',
                        help="Don't include a y-label to the left")
    parser.add_argument('--sup_right_label', action='store_true',
                        help="Don't include a y-label to the right")
    parser.add_argument('--sup_legend', action='store_true',
                        help="Don't include a legend for the names")
    parser.add_argument('--inter_socket', action='store_true',
                        help="Compile to pin threads in intersocket-mode")
    parser.add_argument('--hyperthreading', action='store_true',
                        help="Compile to pin threads in hyperthreading-mode")
    parser.add_argument('--athena_points', action='store_true',
                        help="Use [1, 16, 32 ... 256] as the x-axis")
    parser.add_argument('--dcbl_dra', action='store_true',
                        help="Use the MS d-CBO as d-RA")
    parser.add_argument('--include_start', action='store_true',
                        help="Include the $start argument in x-axis, otherwise stepping backward from $from arg")
    parser.add_argument('--test_timeout', type=int, default=60,
                        help="Set a timeout (in seconds) for each call to a binary before it is aborted [default=60]")
    args = parser.parse_args()
    if args.dcbl_dra:
        RENAME_MAP["dcbl-ms"] = "d-RA"

    return args


if __name__ == '__main__':
    main(parse_args())
