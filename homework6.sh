#!/bin/bash
# Lines beginning with # are comments
# Lines beginning #SBATCH are processed by slurm 

#SBATCH --account=PMIU0184
#SBATCH --job-name=homework6
#SBATCH --time=1:00:00
#SBATCH --mem=2GB
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=8

export OMP_NUM_THREADS=$SLURM_NTASKS

/usr/bin/time -v ./homework1 images/Flag_of_the_US.png images/star_mask.png results/image4.png true 50 32

#end of script