Sandhi
---------

Sandhi is a visual programming software particularly for control systems. Sandhi is forked from GNU Radio project which uses GRAS (GNU Radio Advanced Scheduler) https://github.com/guruofquality/gras/wiki


# Dependencies
Sandhi is conglomeration of lot of libraries and softwares. For computational purposes, Scilab and Octave can be interfaced with Sandhi. Sandhi uses a special wrapper called Sciscipy which alllows one to call scilab functions from python without having to worry about datatype conversion.

To build Sandhi, following dependencies have to be met-


If on Ubuntu one can directly install using command*-


Sciscipy is a must for Sandhi's control blocks to work; To install:
1. Clone sciscipy-1.0.0 from our repository<br>
`git clone https://github.com/manojgudi/sciscipy-1.0.0.git`

2. Run _install_ script from the directory with sudo privileges to automatically pull all dependencies for Sciscipy, build and install it.<br>
`sudo ./install`

*Please note that Sandhi has been known to *not* work libboost1.49 available in Ubuntu 12.10

# Build Guide

<To be copied from GRAS>

# Road Map



