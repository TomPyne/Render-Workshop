#pragma once

// Adapted from https://paulbourke.net/fractals/noise/perlin.h

double PerlinNoise1D(double, double, double, int);
double PerlinNoise2D(double, double, double, double, int);
double PerlinNoise3D(double, double, double, double, double, int);