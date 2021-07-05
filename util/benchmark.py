#!/usr/bin/env python

# Taken from Wren (https://github.com/wren-lang/wren/blob/master/util/benchmark.py)

from __future__ import print_function

import argparse
import math
import os
import os.path
import re
import subprocess
import sys
import json

# Runs the benchmarks.
#
# It runs several benchmarks across several languages. For each
# benchmark/language pair, it runs a number of trials. Each trial is one run of
# a single benchmark script. It spawns a process and runs the script. The
# script itself is expected to output some result which this script validates
# to ensure the benchmark is running correctly. Then the benchmark prints an
# elapsed time. The benchmark is expected to do the timing itself and only time
# the interesting code under test.
#
# This script then runs several trials and takes the best score. (It does
# multiple trials to account for random variance in running time coming from
# OS, CPU rate-limiting, etc.) It takes the best time on the assumption that
# that represents the language's ideal performance and any variance coming from
# the OS will just slow it down.
#
# After running a series of trials the benchmark runner will compare all of the
# language's performance for a given benchmark. It compares by running time
# and score, which is just the inverse running time.
#
# For Next benchmarks, it can also compare against a "baseline". That's a
# recorded result of a previous run of the Next benchmarks. This is useful --
# critical, actually -- for seeing how Next performance changes. Generating a
# set of baselines before a change to the VM and then comparing those to the
# performance after a change is how we track improvements and regressions.
#
# To generate a baseline file, run this script with "--generate-baseline".

NEXT_DIR = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
NEXT_BIN = NEXT_DIR
BENCHMARK_DIR = os.path.join('tests', 'benchmark')

# How many times to run a given benchmark.
NUM_TRIALS = 10

BENCHMARKS = []

# baselines is a dictionary where each baselines are stored for
# each benchmark
# each benchmark again contains a dictionary where each branchname
# name maps to its score in the branch
BASELINES = {}
CURRENT_BRANCH_NAME = "<default>"


def baseline_has_branch(n):
    return BASELINES["branch list"].count(n) == 1


def baseline_branch_count():
    if BASELINES["branch list"] == None:
        BASELINES["branch list"] = []
    return len(BASELINES["branch list"])


def baseline_get_branch_idx(n):
    global BASELINES
    if "branch list" not in BASELINES:
        BASELINES["branch list"] = []
    if BASELINES["branch list"].count(n) == 0:
        BASELINES["branch list"].append(n)
        return len(BASELINES["branch list"]) - 1
    return BASELINES["branch list"].index(n)


def baseline_get_max_branch_length():
    if "branch list" not in BASELINES:
        BASELINES["branch list"] = []
    if len(BASELINES["branch list"]) == 0:
        return 0
    return len(max(BASELINES["branch list"], key=lambda x: len(x)))


def BENCHMARK(name, pattern):
    regex = re.compile(pattern + "\n" + r"elapsed: (\d+\.\d+)", re.MULTILINE)
    BENCHMARKS.append([name, regex, None])

# BENCHMARK("api_call", "true")

# BENCHMARK("api_foreign_method", "100000000")


BENCHMARK("arrays", r"""500000500000""")

BENCHMARK("binary_trees", r"""stretch tree of depth 13 check: -1
8192 trees of depth 4 check: -8192
2048 trees of depth 6 check: -2048
512 trees of depth 8 check: -512
128 trees of depth 10 check: -128
32 trees of depth 12 check: -32
long lived tree of depth 12 check: -1""")


# BENCHMARK("binary_trees_gc", """stretch tree of depth 13 check: -1
# 8192 trees of depth 4 check: -8192
# 2048 trees of depth 6 check: -2048
# 512 trees of depth 8 check: -512
# 128 trees of depth 10 check: -128
# 32 trees of depth 12 check: -32
# long lived tree of depth 12 check: -1""")

BENCHMARK("delta_blue", "14065400")

BENCHMARK("fannkuch_redx", r"""8629
pfannkuchen 9 = 30""")

BENCHMARK("fib", r"""317811
317811
317811
317811
317811""")

BENCHMARK("fibers", r"""4999950000""")

BENCHMARK("garbage_test",
          r"""Final object : <object of 'MyClass'> {2000000, 2000002, 2000002, 2000000}""")

BENCHMARK("mandelbrot", r"""3165191""")

BENCHMARK("map_numeric", r"""2000001000000""")

BENCHMARK("map_string", r"""12799920000""")

BENCHMARK("method_call", r"""true
false""")

BENCHMARK("nbody", r"""-0.16907516382852
-0.16907807066611""")  # 5935 was the original last 4 digits,
# some precision is lost due to value encoding

BENCHMARK("spectral_norm", r"""1.623647098""")

BENCHMARK("string_equals", r"""3000000""")

BENCHMARK("tuples", r"""500000500000""")

BENCHMARK("while", r"""12500002500003""")

LANGUAGES = [
    ("next",           [os.path.join(NEXT_BIN, 'next')], ".n"),
    #("dart",           ["fletch", "run"],                ".dart"),
    ("lua",            ["lua"],                          ".lua"),
    ("luajit (-joff)", ["luajit", "-joff"],              ".lua"),
    ("python",         ["python"],                       ".py"),
    ("python3",        ["python3"],                      ".py"),
    ("ruby",           ["ruby"],                         ".rb")
]

results = {}

if sys.platform == 'win32':
    GREEN = NORMAL = RED = YELLOW = ''
else:
    GREEN = '\033[32m'
    NORMAL = '\033[0m'
    RED = '\033[31m'
    YELLOW = '\033[33m'


def green(text):
    return GREEN + text + NORMAL


def red(text):
    return RED + text + NORMAL


def yellow(text):
    return YELLOW + text + NORMAL


def get_score(time):
    """
    Converts time into a "score". This is the inverse of the time with an
  arbitrary scale applied to get the number in a nice range. The goal here is
  to have benchmark results where faster = bigger number.
    """
    return 1000.0 / time


def standard_deviation(times):
    """
    Calculates the standard deviation of a list of numbers.
    """
    mean = sum(times) / len(times)

    # Sum the squares of the differences from the mean.
    result = 0
    for time in times:
        result += (time - mean) ** 2

    return math.sqrt(result / len(times))


def run_trial(benchmark, language):
    """Runs one benchmark one time for one language."""
    executable_args = language[1]

    # Hackish. If the benchmark name starts with "api_", it's testing the Next
    # C API, so run the test_api executable which has those test methods instead
    # of the normal Next build.
    # if benchmark[0].startswith("api_"):
    #  executable_args = [
    #    os.path.join(NEXT_DIR, "build", "release", "test", "api_next")
    #  ]

    args = []
    args.extend(executable_args)
    args.append(os.path.join(BENCHMARK_DIR, benchmark[0] + language[2]))

    try:
        out = subprocess.check_output(args, universal_newlines=True)
    # `print("{" + out + "}\n")
    except OSError:
        print('Interpreter was not found')
        return None
    except subprocess.CalledProcessError as e:
        print("[Error] Interpreter exited with a non zero status!")
        print("Output: ")
        print(e.output)
        return None
    match = benchmark[1].match(out)
    if match:
        return float(match.group(1))
    else:
        print("Incorrect output:")
        print(out)
        return None


def run_benchmark_language(benchmark, language, benchmark_result):
    """
    Runs one benchmark for a number of trials for one language.

  Adds the result to benchmark_result, which is a map of language names to
  results.
    """

    if not os.path.exists(os.path.join(
            BENCHMARK_DIR, benchmark[0] + language[2])):
        #print("No implementation for this language")
        # print()
        return

    name = "{0} - {1}".format(benchmark[0], language[0])
    print("{0:20s}".format(name), end='  ')

    if NUM_TRIALS < 3:
        print("{:^{}}".format(" ", 3 - NUM_TRIALS - 1), end='')

    times = []
    for i in range(0, NUM_TRIALS):
        sys.stdout.flush()
        time = run_trial(benchmark, language)
        if not time:
            return
        times.append(time)
        sys.stdout.write(".")

    print("  ", end='')

    best = min(times)
    score = get_score(best)

    comparison = ""
    # print(max_width)
    if language[0] == "next":
        all_scores = benchmark[2]
        comparison = ""
        i = 0
        for ascore in all_scores:
            max_width = max(8, len(BASELINES["branch list"][i]))
            if ascore != None:
                ratio = 100 * score / ascore
                ratio_str = "{:^6.2f}%".format(ratio)
                ratio_str = "{:^{}}".format(ratio_str, max_width)
                if ratio > 105:
                    ratio_str = green(ratio_str)
                if ratio < 95:
                    ratio_str = red(ratio_str)
                comparison += ratio_str
            else:
                comparison += "{:^{}}".format("---", max_width)
            comparison += '  '
            i += 1
    else:
        # Hack: assumes next gets run first.
        next_score = benchmark_result["next"]["score"]
        ratio = 100.0 * next_score / score
        comparison = "{:^6.2f}%".format(ratio)
        if ratio > 105:
            comparison = green(comparison)
        if ratio < 95:
            comparison = red(comparison)

    print("{:4.2f}s   {:4.4f}   {:s}".format(
        best,
        standard_deviation(times),
        comparison))

    benchmark_result[language[0]] = {
        "desc": name,
        "times": times,
        "score": score
    }

    return score


def run_benchmark(benchmark, languages, graph):
    """Runs one benchmark for the given languages (or all of them)."""

    benchmark_result = {}
    results[benchmark[0]] = benchmark_result

    num_languages = 0
    for language in LANGUAGES:
        if not languages or language[0] in languages:
            num_languages += 1
            run_benchmark_language(benchmark, language, benchmark_result)

    if num_languages > 1 and graph:
        graph_results(benchmark_result)


def graph_results(benchmark_result):
    print()

    INCREMENT = {
        '-': 'o',
        'o': 'O',
        'O': '0',
        '0': '0'
    }

    # Scale everything by the highest score.
    highest = 0
    for language, result in benchmark_result.items():
        score = get_score(min(result["times"]))
    if score > highest:
        highest = score

    print("{0:30s}0 {1:66.0f}".format("", highest))
    for language, result in benchmark_result.items():
        line = ["-"] * 68
    for time in result["times"]:
        index = int(get_score(time) / highest * 67)
        line[index] = INCREMENT[line[index]]
    print("{0:30s}{1}".format(result["desc"], "".join(line)))
    print()


def read_baseline():
    global BASELINES
    baseline_file = os.path.join(BENCHMARK_DIR, "baseline.json")
    if os.path.exists(baseline_file):
        with open(baseline_file) as f:
            BASELINES = json.load(f)
    for benchmark in BENCHMARKS:
        if benchmark[0] in BASELINES:
            all_scores = BASELINES[benchmark[0]]
            benchmark[2] = all_scores
        else:
            benchmark[2] = []


def get_branch_name(b):
    if b != None:
        return b
    out = "<default>"
    try:
        out = subprocess.check_output(
            ["git", "rev-parse", "--abbrev-ref", "HEAD"], universal_newlines=True)[:-1]
    except:
        pass
    return out


def resize(l, newsize, filling=None):
    if newsize > len(l):
        l.extend([filling for x in range(len(l), newsize)])


def print_header():
    # print the header
    print("{:^20s}".format("Name"), end='  ')
    print("{:^{}s}".format("Run", NUM_TRIALS), end='  ')
    print("{:^6s}".format("Best"), end='  ')
    print("{:^6s}".format("SD"), end='  ')
    maxlen = baseline_get_max_branch_length()
    for i in BASELINES["branch list"]:
        print("{:^{}s}".format(i, max(8, len(i))), end='  ')
    print()
    print("{:-^20s}".format(""), end='  ')
    print("{:-^{}s}".format("---", NUM_TRIALS), end='  ')
    print("{:-^6s}".format(""), end='  ')
    print("{:-^6s}".format(""), end='  ')
    for i in BASELINES["branch list"]:
        print("{:-^{}s}".format("", max([8, len(i)])), end='  ')
    print()


def write_baseline():
    # Write them to a file.
    baseline_file = os.path.join(BENCHMARK_DIR, "baseline.json")
    with open(baseline_file, 'w') as out:
        json.dump(BASELINES, out)


def generate_baseline():
    global BASELINES
    print("Generating baseline for branch '", CURRENT_BRANCH_NAME, "'..")
    print_header()
    idx = baseline_get_branch_idx(CURRENT_BRANCH_NAME)
    for benchmark in BENCHMARKS:
        best = run_benchmark_language(benchmark, LANGUAGES[0], {})
        if benchmark[0] not in BASELINES:
            BASELINES[benchmark[0]] = []
        resize(BASELINES[benchmark[0]], idx + 1)
        BASELINES[benchmark[0]][idx] = best
    write_baseline()


def print_html():
    '''Print the results as an HTML chart.'''

    def print_benchmark(benchmark, name):
        print('<h3>{}</h3>'.format(name))
    print('<table class="chart">')

    # Scale everything by the highest time.
    highest = 0
    for language, result in results[benchmark].items():
        time = min(result["times"])
        if time > highest:
            highest = time

    languages = sorted(results[benchmark].keys(),
                       key=lambda lang: results[benchmark][lang]["score"], reverse=True)

    for language in languages:
        result = results[benchmark][language]
        time = float(min(result["times"]))
        ratio = int(100 * time / highest)
        css_class = "chart-bar"
        if language == "next":
            css_class += " next"
            print('  <tr>')
            print('    <th>{}</th><td><div class="{}" style="width: {}%;">{:4.2f}s&nbsp;</div></td>'.format(
                language, css_class, ratio, time))
            print('  </tr>')
    print('</table>')

    print_benchmark("method_call", "Method Call")
    print_benchmark("delta_blue", "DeltaBlue")
    print_benchmark("binary_trees", "Binary Trees")
    print_benchmark("fib", "Recursive Fibonacci")


def main():
    global NUM_TRIALS
    global CURRENT_BRANCH_NAME
    parser = argparse.ArgumentParser(description="Run the benchmarks")
    parser.add_argument("benchmark", nargs='?',
                        default="all",
                        help="The benchmark to run")
    parser.add_argument("--generate-baseline",
                        action="store_true",
                        help="Generate a baseline file")
    parser.add_argument("--graph",
                        action="store_true",
                        help="Display graph results.")
    parser.add_argument("-l", "--language",
                        action="append",
                        help="Which language(s) to run benchmarks for")
    parser.add_argument("--output-html",
                        action="store_true",
                        help="Output the results chart as HTML")
    parser.add_argument("-n", "--numtrials",
                        help="Specify the number of trials",
                        type=int, nargs=1, default=[10])
    parser.add_argument("-b", "--branch",
                        help="Specify the current branch name",
                        type=str, nargs=1, default=[None])
    parser.add_argument("-r", "--remove-baseline",
                        help="Remove a baseline from results",
                        type=str, nargs=1, default=[None])
    args = parser.parse_args()

    NUM_TRIALS = args.numtrials[0]
    CURRENT_BRANCH_NAME = get_branch_name(args.branch[0])

    read_baseline()

    rem = args.remove_baseline[0]
    if rem != None:
        if rem in BASELINES["branch list"]:
            idx = BASELINES["branch list"].index(rem)
            del BASELINES["branch list"][idx]
            for benchmark in BENCHMARKS:
                if benchmark[0] in BASELINES and len(BASELINES[benchmark[0]]) > idx:
                    del BASELINES[benchmark[0]][idx]
            write_baseline()
            read_baseline()

    if args.generate_baseline:
        generate_baseline()
        return

    print_header()
    # Run the benchmarks.
    for benchmark in BENCHMARKS:
        if benchmark[0] == args.benchmark or args.benchmark == "all":
            run_benchmark(benchmark, args.language, args.graph)

    if args.output_html:
        print_html()


main()
