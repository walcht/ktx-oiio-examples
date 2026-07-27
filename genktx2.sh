#! /bin/bash

# Regenerate the input ktx2

ktx create --testrun --threads 1 --encode uastc --format R8G8B8_SRGB --assign-tf srgb --assign-primaries bt709 resources/png/basis_uastc.png resources/ktx2/2d_uastc.ktx2

