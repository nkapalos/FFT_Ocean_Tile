#pragma once

//================================================================================
// #Defines to test different execution configurations.
// Proposed configuration:
//
// #define SIZE_OF_GRID 512
// #define GPGPU_NORM_CD
//================================================================================

// Size of grid.
#define SIZE_OF_GRID 512		// Grid size 512
//#define SIZE_OF_GRID 256		// Grid size 256

// Execution Configurations. Choose only one at a time.
#define GPGPU_NORM_CD			// Usage of GPGPU, Normals calculated with central difference.
//#define CPU_NORM_FFT			// No usage of GPGPU, Normals calculated with additional FFTs.
//#define CPU_NORM_CD			// No usage of GPGPU, Normals calculated with central difference.