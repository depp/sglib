#!/usr/bin/env python3
# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
"""Test for mixer timing.

We feed the mixer some data with random delays, and check that the
predicted delay matches the expected delay.
"""
import subprocess
import numpy
import math

class TestFailure(Exception):
    pass

def test(bufsize, data, *, deltabits=None, mixahead=None, safety=None,
         maxslope=None, smoothing=None):
    if not isinstance(data, numpy.ndarray):
        raise TypeError('data must be ndarray')
    if data.dtype != numpy.uint32 and data.dtype != numpy.int32:
        raise TypeError('data must contain uint32 or int32')
    if len(data.shape) != 2:
        raise TypeError('data has must be 2D')
    if data.shape[1] != 2:
        raise TypeError('data must have 2 columns')
    if not data.flags['C_CONTIGUOUS']:
        raise TypeError('data must be contiguous')
    cmd = [
        './mixer_time',
        '{:d}'.format(bufsize),
        '-' if deltabits is None else '{:d}'.format(deltabits),
        '-' if mixahead  is None else '{:d}'.format(mixahead),
        '-' if safety    is None else '{:f}'.format(safety),
        '-' if maxslope  is None else '{:f}'.format(maxslope),
        '-' if smoothing is None else '{:f}'.format(smoothing),
    ]
    proc = subprocess.Popen(
        cmd,
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE)
    stdout, stderr = proc.communicate(data.tostring())
    if proc.returncode != 0:
        raise TestFailure(
            'Program returned {}\n{}'
            .format(proc.returncode))
    return numpy.fromstring(stdout, dtype=numpy.uint32)

def test_simple(*, bufsize, count, speed, mindelay, maxdelay):
    realtime = numpy.linspace(
        bufsize,
        count * bufsize / speed + bufsize,
        num=count + 1)
    realtime = realtime.astype(numpy.int32)
    indata = numpy.ndarray((count + 1, 2), numpy.int32)
    indata[:,0] = realtime - numpy.random.random_integers(
        mindelay + bufsize, maxdelay + bufsize, count + 1)
    indata[:,1] = realtime
    outdata = test(bufsize, indata)
    delay = ((outdata.astype(numpy.float64)) * (1 / speed) -
             numpy.arange(len(outdata), dtype=numpy.float64))
    start = int(bufsize * 32 / speed + 2**14)
    delay = delay[start:]
    mean = numpy.mean(delay)
    var = numpy.var(delay)
    print('Delay: {:.0f} +/- {:.0f}'.format(mean, math.sqrt(var)))

    dmean = (mindelay + maxdelay) / 2
    dvar = (maxdelay - mindelay)**2 / 12
    print('Expected: {:.0f}'
          .format(bufsize + dmean + 2 * math.sqrt(dvar) + (bufsize >> 3)))

test_simple(
    bufsize=1024,
    count=500,
    speed=1.1,
    mindelay=20,
    maxdelay=600,
)

test_simple(
    bufsize=128,
    count=2000,
    speed=1.1,
    mindelay=20,
    maxdelay=600,
)
