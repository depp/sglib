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

def test(rate, bufsize, data, *,
         deltabits=None, mixahead=None, safety=None,
         maxslope=None, smoothing=None):
    if not isinstance(data, numpy.ndarray):
        raise TypeError('data must be ndarray')
    if data.dtype != numpy.float64:
        raise TypeError('data must contain float64')
    if len(data.shape) != 2:
        raise TypeError('data has must be 2D')
    if data.shape[1] != 2:
        raise TypeError('data must have 2 columns')
    if not data.flags['C_CONTIGUOUS']:
        raise TypeError('data must be contiguous')
    cmd = [
        './mixer_time',
        '{:d}'.format(rate),
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
            .format(proc.returncode, stderr))
    out = numpy.fromstring(stdout, dtype=numpy.float64)
    return out.reshape((out.size//2, 2))

def test_simple(*, rate, bufsize, count, speed, mindelay, maxdelay):
    buftime = bufsize / (rate * speed)
    realtime = numpy.linspace(buftime, (count + 1) * buftime, num=count + 1)
    indata = numpy.ndarray((count + 1, 2), numpy.float64)
    delay = ((mindelay + buftime) +
             (maxdelay - mindelay) * numpy.random.random(count + 1))
    indata[:,0] = realtime - delay
    indata[:,1] = realtime
    del delay
    outdata = test(rate, bufsize, indata)
    del indata

    warmuptime = 32 * bufsize / (rate * speed) + 0.125
    index = numpy.argmax(outdata[:,0] > warmuptime)
    delay = outdata[index:,1] * (1.0 / (rate * speed)) - outdata[index:,0]

    print('Delay: {:.1f} ms \xb1 {:.1f} ms ({:.1f} - {:.1f} ms)'.format(
        numpy.mean(delay) * 1000,
        math.sqrt(numpy.var(delay)) * 1000,
        numpy.min(delay) * 1000,
        numpy.max(delay) * 1000))

    expected = (
        (mindelay + maxdelay) / 2 +
        buftime * 1.125 +
        2.0 * (maxdelay - mindelay) / math.sqrt(12))
    print('Expected: {:.0f} ms'
          .format(expected * 1000))

test_simple(
    rate     = 48000,
    bufsize  = 1024,
    count    = 500,
    speed    = 1.1,
    mindelay = 0.001,
    maxdelay = 0.010,
)

test_simple(
    rate     = 48000,
    bufsize  = 128,
    count    = 2000,
    speed    = 1.1,
    mindelay = 0.001,
    maxdelay = 0.010,
)
