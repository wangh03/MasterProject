#!/bin/bash
#PBS -N fieldopt_testrun
#PBS -lnodes=1:ppn=12:default
#PBS -lwalltime=00:1:00
#PBS -lpmem=200MB
#PBS -A acc-ipt
#PBS -q default

cd ${PBS_O_WORKDIR}/build/Console
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:../src

module load intelcomp/13.0.1
module load openmpi/1.6.5
module load python/2.7.6-intel
module load boost
module load qt

mpirun -npernode 12 -x LD_LIBRARY_PATH Console
