#pragma once

// Import the header.
#include <cstdint>
#include <cmath>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>

#include <omp.h>

#include "Configurations.h"
#include "hr_time.h"

#include "fftw3.h"
#pragma comment(lib, "libfftw3f-3.lib")

struct Vec2
{
	float x;
	float y;
};

class FFTWrapper
{

// Singleton Pattern
public:
	static FFTWrapper& getInstance(const int& gridsize)
	{
		static FFTWrapper instance(gridsize);
		return instance;
	}

private:
	FFTWrapper(const int& gridsize);

public:
	FFTWrapper(FFTWrapper const&) = delete; // Don't Implement
	void operator=(FFTWrapper const&) = delete;   // Don't Implement
	FFTWrapper() = delete;	// Don't Implement

	~FFTWrapper();	

// Variables and Constants
private:
	// Constants
	const float kfPi = 3.1415926f;
	const float kfTwoPi = 6.283185307f;
	const float kfGravity = 9.81f;
	const float kfWorldUnit = 200;	

	const unsigned int m_width;
	const unsigned int m_height;
	const unsigned int m_pngChannels = 4;

	// Philips Spectrum presets
	Vec2 m_windNormal = { 1, 0 };
	float m_windSpeed = 26.0f;
	float m_amplitude = 20.0f;

	// Arrays used in initialisation
	Vec2* m_kVectors;
	float* m_kMag;

	Vec2* m_h0tilde;
	Vec2* m_h0tildeConj;

	// FFT input and output arrays
	fftwf_complex* m_FFTin[3];
	fftwf_complex* m_FFTout[3];

	// FFT plans
	fftwf_plan m_plan[3];

	// Sinusoids precalculation
	int* m_cosLookup;
	int* m_sinLookup;
	std::vector<float>* m_cosPrecalc;
	std::vector<float>* m_sinPrecalc;

	// Textures in array form
	float* pImageOut;
	float* pNormalOut;

// Getter Methods
public:
	inline const int& getWidth() { return m_width; }
	inline const int& getHeight() { return m_height; }

	inline float* getKMag() { return m_kMag; }
	inline Vec2* getH0Tilde() { return m_h0tilde; }
	inline Vec2* getH0TildeConj() { return m_h0tildeConj; }
	inline float* getImageOut() { return pImageOut; }
	inline float* getNormalOut() { return pNormalOut; }
	inline fftwf_complex* getFFTin(const int& index) { return m_FFTin[index]; }

// FFT Methods
public:
	// Initialisation
	void Generate_Heightmap();

	// Model initialisation functions
	float Philips_Spectrum(const Vec2& vK, float& kMag);
	void Fill_K_Vectors();
	void Fill_h0tilde();
	void Precalculate_Sinusoids();

	// Preparation for IFFT every frame
	void Fill_htilde_and_Displacements();
	void Fill_Horizontal_Displacement();

	// Parallel IFFT execution
	void IFFT_Thread();

	// Fill heightmap Texture to feed to GPU
	void Fill_Texture();

	// Fill normalmap Texture to feed to GPU
	void Fill_Normals_FFT(const float& choppy, const float& foamInt);
	void Fill_Normals_Central_Diff(const float& choppy, const float &heightAdj, const float& foamInt);

// Maths
public:
	inline float magnitude(const fftwf_complex& v)
	{
		return sqrt(v[0] * v[0] + v[1] * v[1]);
	}
	
	inline float sqr(float x)
	{
		return x*x;
	}

	inline float pow4(float x)
	{
		return x*x*x*x;
	}

	inline float dot(const Vec2& a, const Vec2& b)
	{
		return a.x*b.x + a.y*b.y;
	}

	inline float magnitude(const Vec2& v)
	{
		return sqrt(v.x * v.x + v.y * v.y);
	}

	inline Vec2 normal(const Vec2& v)
	{
		float temp = sqrt(v.x * v.x + v.y * v.y);
		return{ v.x / temp, v.y / temp };
	}

	inline Vec2 normal(const Vec2& v, const float& kMag)
	{
		return{ v.x / kMag, v.y / kMag};
	}

	inline Vec2 multComplex(const Vec2& a, const Vec2& b)
	{
		return{ a.x * b.x - a.y * b.y , a.x * b.y + a.y * b.x };
	}

	inline Vec2 addComplex(const Vec2& a, const Vec2& b)
	{
		return{ a.x + b.x , a.y + b.y };
	}

	inline float randf1()
	{
		return (float)rand() / (float)RAND_MAX;
	}

	inline float clip(float v, float max)
	{
		return v < max ? v : max;
	}

	inline float clamp(float x, float lowerlimit, float upperlimit)
	{
		if (x < lowerlimit)
			x = lowerlimit;
		if (x > upperlimit)
			x = upperlimit;
		return x;
	}

	inline float mapTo01(float x, float edge0, float edge1)
	{
		return clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
	}

	inline float smoothstep(float x, float edge0, float edge1) 
	{
		mapTo01(x, edge0, edge1);
		return x * x * (3 - 2 * x);
	}
};