# FFT_Ocean_Tile
This project comprises a modern adaptation of Jerry Tessendorf’s popular paper “Simulating Ocean Water”. The goal was to create a prototype of a single tile of realistic oceanic surface using C++ and DirectX11. The waves are generated procedurally in real time, initially storing data generated from the oceanographic model Philips Spectrum, and then calculating every frame the waves realization using the Fast Fourier Transform (FFT).

The scope of the project includes the implementation of the mathematical models for the generation of realistic wave shapes, shading using HLSL and DX11, optimisation using compute shaders, and creation of a customisable UI for artistic experimentation using ImGui. For the execution of the Fast Fourier Transform the [FFTW library](http://www.fftw.org/) was used.

## Configurations
To switch between versions, comment/uncomment the appropriate #define directives in the header Configurations.h.

### To run the application from Visual Studio:
* Set Solution Configuration to "Release x86".
* Set AppOcean as StartUp Project.

### To run as a standalone
* Make sure the Assets folder is present.
* Make sure libfftw3f-3.dll is present.

## Next Steps
* Test the performance of a GPU FFT implementation, to avoid costly data transfer to the CPU.
* Implement distance-dependent tesselation and LODs.

## Acknowledgements
* This project was created as part of the module "Research and Development Project" of the Master's course Games Software Development at Sheffield Hallam University
* The DirectX framework for this project was provided by Dr. David Moore
