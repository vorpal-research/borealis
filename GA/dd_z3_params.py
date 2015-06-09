#!/usr/bin/env python

import collections
import math
import random
import re
import subprocess
import sys
import time

from collections import namedtuple, OrderedDict
from itertools import takewhile

Param = namedtuple('Param', ['key', 'value', 'ttype'])

class Params:
    _rex = re.compile(r"(.*)\s=\s(.*)\s(\(.*\))")

    def __init__(self):
        self._storage = OrderedDict()

    def load(self, fname):
        with open(fname, "r") as ffile:
            for line in ffile:
                match = self._rex.match(line)
                p = Param(match.group(1), match.group(2), match.group(3))
                self._storage[p.key] = p
        return self

    def dump(self, fname):
        with open(fname, "w") as ffile:
            for param in self._storage.values():
                ffile.write("{0} = {1} {2}\n".format(param.key, param.value, param.ttype))
        return self

    def copy(self):
        res = Params()
        res._storage = self._storage.copy()
        return res

    def __cmp__(self, obj):
        if obj is None: return 1
        if not isinstance(obj, Params): return 1
        return self._storage.__cmp__(obj._storage)


def run_tests():
    cmd = "timeout 8s ./run-tests --gtest_output='xml:test_results.xml' --gtest_color=yes --gtest_filter=Necla/*"
    start = time.perf_counter()
    ret = subprocess.call(cmd.split(), stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    end = time.perf_counter()
    return end - start if 0 == ret else 4294967295


class DD:
    _rand = random.Random()
    _rand.seed()

    def __init__(self):
        self._base = Params().load("z3.params.defaults")
        self._target = Params().load("z3.params.0")

    @staticmethod
    def diff(a, b):
        zipped = zip(a._storage.values(), b._storage.values())

        diff = []
        for after, before in zipped:
            if after.key != before.key:
                raise LookupError("Key mismatch: {0} vs {1}".format(after.key, before.key))
            if after != before:
                diff.append(after.key)

        return diff

    def doit(self):
        base = self._base
        target = self._target

        diff = DD.diff(base, target)
        while len(diff) > 5:
            cand = target.copy()

            for key in diff:
                if self._rand.random() < 0.1:
                    cand._storage[key] = base._storage[key]

            target.dump("z3.params")
            tt = run_tests()
            cand.dump("z3.params")
            tc = run_tests()

            if math.fabs(tt / tc - 1.0) < 0.005:
                target = cand

                old = len(diff)
                diff = DD.diff(base, target)
                new = len(diff)
                print("Improved from {0} to {1}".format(old, new))
                print("{0} -> {1}".format(tt, tc))

            target.dump("z3.params.dd")

def main():
    dd = DD()
    dd.doit()

if __name__ == "__main__":
    main()
