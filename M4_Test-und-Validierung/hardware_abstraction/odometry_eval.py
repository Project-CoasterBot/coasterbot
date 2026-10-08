#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Thu Oct  8 09:20:51 2026

@author: gereon
"""


import numpy
import matplotlib.pyplot as plt

lines = open('serial_log.txt').read().splitlines()
lines = [l for l in lines if l.startswith('[COORD]')]

lines = [l.split('heading=')[1] for l in lines]


lines = [[float(l.split()[k]) for k in (0,1,3)] for l in lines]

a = numpy.array(lines)

h = a[:,0] / (numpy.pi*2) * 360
x = a[:,1]
y = a[:,2]


t = numpy.arange(a.shape[0]) * 0.5

fig, ax1 = plt.subplots()

color = 'tab:blue'
ax1.set_xlabel('time (s)')
ax1.plot(t, x, color=color, label='x')
ax1.tick_params(axis='y', labelcolor=color)

color = 'tab:red'
ax1.plot(t, y, color=color, label='y')
ax1.tick_params(axis='y', labelcolor=color)



ax2 = ax1.twinx()  # instantiate a second Axes that shares the same x-axis

color = 'tab:green'
ax2.set_ylabel('heading', color=color)  # we already handled the x-label with ax1
ax2.plot(t, h, color=color, label='heading')
ax2.tick_params(axis='y', labelcolor=color)

fig.tight_layout()  # otherwise the right y-label is slightly clipped
plt.show()



plt.show()