#include "FFTWrapper.h"
#include "Framework.h"
#include <random>			// For Gaussian Samples
#include <fstream>			// For File Output

FFTWrapper::FFTWrapper(const int& gridsize)
	:m_height(gridsize), m_width(gridsize)
{
	m_kVectors = new Vec2[m_width * m_height];
	m_kMag = new float[m_width * m_height];

	m_h0tilde = new Vec2[m_width * m_height];
	m_h0tildeConj = new Vec2[m_width * m_height];

	m_cosLookup = new int[m_width * m_height];
	m_sinLookup = new int[m_width * m_height];
	m_cosPrecalc = new std::vector<float>[m_width * m_height];
	m_sinPrecalc = new std::vector<float>[m_width * m_height];

	pImageOut = new float[m_width * m_height * 4];
	pNormalOut = new float[m_width * m_height * 4];

	m_FFTin[0] = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * m_width * m_height);
	m_FFTin[1] = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * m_width * m_height);
	m_FFTin[2] = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * m_width * m_height);
	m_FFTout[0] = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * m_width * m_height);
	m_FFTout[1] = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * m_width * m_height);
	m_FFTout[2] = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * m_width * m_height);

	// FFTW parallelism
	fftwf_init_threads();
	fftwf_plan_with_nthreads(omp_get_max_threads());

	m_plan[0] = fftwf_plan_dft_2d(m_width, m_height, m_FFTin[0], m_FFTout[0], FFTW_BACKWARD, FFTW_ESTIMATE);
	m_plan[1] = fftwf_plan_dft_2d(m_width, m_height, m_FFTin[1], m_FFTout[1], FFTW_BACKWARD, FFTW_ESTIMATE);
	m_plan[2] = fftwf_plan_dft_2d(m_width, m_height, m_FFTin[2], m_FFTout[2], FFTW_BACKWARD, FFTW_ESTIMATE);

}

FFTWrapper::~FFTWrapper()
{
	delete[] m_kVectors;
	delete[] m_kMag;

	delete[] m_h0tilde;
	delete[] m_h0tildeConj;

	delete[] m_cosLookup;
	delete[] m_sinLookup;
	delete[] m_cosPrecalc;
	delete[] m_sinPrecalc;

	delete[] pImageOut;
	delete[] pNormalOut;

	fftwf_destroy_plan(m_plan[2]);
	fftwf_destroy_plan(m_plan[1]);
	fftwf_destroy_plan(m_plan[0]);

	fftwf_free(m_FFTin[0]);
	fftwf_free(m_FFTin[1]);
	fftwf_free(m_FFTin[2]);
	fftwf_free(m_FFTout[0]);
	fftwf_free(m_FFTout[1]);
	fftwf_free(m_FFTout[2]);

}

float FFTWrapper::Philips_Spectrum(const Vec2& vK, float& kMag)
{
	// Philips Spectrum calculation for specific k Vector
	float L = (m_windSpeed * m_windSpeed) / kfGravity;
	float k2 = sqr(kMag);
	float normDot = sqr(dot(normal(vK, kMag), m_windNormal)); 
	float numer = exp(-1.0f / (k2 * sqr(L)));
	float denom = sqr(k2);
	return m_amplitude * (numer / denom) * normDot;
}

void FFTWrapper::Fill_K_Vectors()
{
	// Fill the k Vectors array
	int height2 = m_height / 2;
	int width2 = m_width / 2;
	uint32_t n = 0;
	
	float kz, kx;

	for (int j = 0; j < m_height; ++j)
	{
		kz = kfTwoPi * (j) / kfWorldUnit;
		for (int i = 0; i < m_width; ++i)
		{
			kx = kfTwoPi * (i) / kfWorldUnit;

			m_kVectors[n] = { kx, kz };
			m_kMag[n] = magnitude(m_kVectors[n]);

			// Avoid division by zero
			if (m_kMag[n] < 0.0001f)
				m_kMag[n] = 0.0001f;

			++n;
		}
	}
}

void FFTWrapper::Precalculate_Sinusoids()
{
	// Precalculate all sinusoids on the first frame to
	// calculate the exp terms using Euler's formula. 
	// Variable "step" works like timescale

	float omegaK = 0;
	float wavePeriod = 0;
	float step = 0.05f;

	// For the CPU implementation, we can precalculate all sinusoids
	// and store them into vectors of different sizes.
	// This however disables timescaling, so another solution should be 
	// found if the game needs to support that.

	memset(m_cosLookup, 0, m_width * m_height * sizeof(int));
	memset(m_sinLookup, 0, m_width * m_height * sizeof(int));

	for (uint32_t i(0); i < (m_width * m_height); ++i)
	{
		omegaK = sqrt(kfGravity * m_kMag[i]);

		wavePeriod = kfTwoPi / omegaK;

		m_cosPrecalc[i].clear();
		m_sinPrecalc[i].clear();

		// Every wave has a different period, so to precalculate them
		// we need to precalculate different amounts of "screenshots" for each wave
		for (float time_unit(0); time_unit < wavePeriod; time_unit += step)
		{
			m_cosPrecalc[i].push_back(cos(omegaK * time_unit));
			m_sinPrecalc[i].push_back(sin(omegaK * time_unit));
		}
	}
}

void FFTWrapper::Fill_h0tilde()
{
	std::random_device rd;
	std::mt19937 e2(rd());

	// Gaussian distribution with mean 0 and standard deviation 1
	std::normal_distribution<> dist(0, 1);

	const float oneOverRoot2 = 1.0f / sqrt(2.f);

	for (uint32_t n(0); n < (m_width * m_height); ++n)
	{
		// Fill h0tilde
		float rootOfPh = sqrt(Philips_Spectrum(m_kVectors[n], m_kMag[n]));
		rootOfPh *= oneOverRoot2;
		m_h0tilde[n] = { (float)dist(e2) * rootOfPh, (float)dist(e2) * rootOfPh };

		// Fill h0tildeConjugate
		rootOfPh = sqrt(Philips_Spectrum({ -m_kVectors[n].x , -m_kVectors[n].y }, m_kMag[n]));
		rootOfPh *= oneOverRoot2;
		m_h0tildeConj[n] = { (float)dist(e2) * rootOfPh, -(float)dist(e2) * rootOfPh };
	}
}

void FFTWrapper::Fill_htilde_and_Displacements()
{
	Vec2 expPos, expNeg, temp;
	Vec2 htilde, dispX, dispZ;
	float oneOverKMag;

	for (uint32_t i(0); i < (m_width * m_height); ++i)
	{
		oneOverKMag = 1.0f / m_kMag[i];

		// Lookup the precalculated sin and cos values
		++m_cosLookup[i];
		++m_sinLookup[i];

		if (m_cosLookup[i] >= m_cosPrecalc[i].size())
			m_cosLookup[i] = 0;
		if (m_sinLookup[i] >= m_sinPrecalc[i].size())
			m_sinLookup[i] = 0;

		// exp values calculated with Euler's formula
		expPos = { m_cosPrecalc[i][m_cosLookup[i]], m_sinPrecalc[i][m_sinLookup[i]] };
		expNeg = { expPos.x, -expPos.y };

		// Fill htilde
		htilde = addComplex(multComplex(m_h0tilde[i], expPos), multComplex(m_h0tildeConj[i], expNeg));
		m_FFTin[0][i][0] = htilde.x;	
		m_FFTin[0][i][1] = htilde.y;

		// Fill Displacement (X-Axis)
		dispX = multComplex({ 0, -m_kVectors[i].x * oneOverKMag }, htilde);
		m_FFTin[1][i][0] = dispX.x;	
		m_FFTin[1][i][1] = dispX.y;

		// Fill Displacement (Z-Axis)
		dispZ = multComplex({ 0, -m_kVectors[i].y * oneOverKMag }, htilde);
		m_FFTin[2][i][0] = dispZ.x;	
		m_FFTin[2][i][1] = dispZ.y;	
	}
}

void FFTWrapper::Fill_Horizontal_Displacement()
{
	float oneOverKMag;

	// Fill horizontal displacement only.

	for (uint32_t i(0); i < (m_width * m_height); ++i)
	{
		oneOverKMag = 1.0f / m_kMag[i];

		m_FFTin[1][i][0] = (1) * m_kVectors[i].x * m_FFTin[0][i][1] * oneOverKMag;
		m_FFTin[1][i][1] = (-1) * m_kVectors[i].x * m_FFTin[0][i][0] * oneOverKMag;

		m_FFTin[2][i][0] = (1) * m_kVectors[i].y * m_FFTin[0][i][1] * oneOverKMag;
		m_FFTin[2][i][1] = (-1) * m_kVectors[i].y * m_FFTin[0][i][0] * oneOverKMag;
	}
}

void FFTWrapper::Fill_Texture()
{
	for (uint32_t n(0); n < m_height * m_width; ++n)
	{

#define SHOWFFT
//#define SHOWHTILDE

#ifdef SHOWFFT

		pImageOut[4 * n + 0] = (m_FFTout[1][n][0]) / (m_height);	//X
		pImageOut[4 * n + 1] = (m_FFTout[0][n][0]) / (m_height);	//Y
		pImageOut[4 * n + 2] = (m_FFTout[2][n][0]) / (m_height);	//Z
		pImageOut[4 * n + 3] = 1;

#endif // SHOWFFT

		// Only for debugging purposes
#ifdef SHOWHTILDE
		// htilde representation in the frequency domain
		pImageOut[4 * n + 0] = (m_FFTin[0][n][0]) * 50;	
		pImageOut[4 * n + 1] = (m_FFTin[0][n][1]) * 50;	
		pImageOut[4 * n + 2] = 0;
		pImageOut[4 * n + 3] = 1;
#endif // SHOWHTILDE
	}
}

void FFTWrapper::Fill_Normals_FFT(const float& choppy, const float& foamInt)
{
	uint32_t n;
	int xNext, zNext, nInd;
	float s11, s21, s12;
	float Jxx, Jyy, Jxy, Jyx, jacobian;

	float intensity = 1 / (m_height / (1 + foamInt));
	intensity = (foamInt == 0) ? 0 : intensity;

	for (int j(0); j < m_height; ++j)
	{
		for (int i(0); i < m_width; ++i)
		{
			// Jacobian Calculation
			xNext = (i == m_width - 1) ? xNext = 0 : xNext = i + 1;
			zNext = (j == m_height - 1) ? zNext = 0 : zNext = j + 1;

			n = j * m_height + i;

			nInd = j * m_height + xNext;
			Jxx = 1 + choppy * (-m_FFTout[1][nInd][0] + m_FFTout[1][n][0]) * intensity;
			Jxy = choppy * (-m_FFTout[2][nInd][0] + m_FFTout[2][n][0]) * intensity;

			nInd = zNext * m_height + i;
			Jyy = 1 + choppy * (-m_FFTout[2][nInd][0] + m_FFTout[2][n][0]) * intensity;
			Jyx = choppy * (-m_FFTout[1][nInd][0] + m_FFTout[1][n][0]) * intensity;

			jacobian = (Jxx * Jyy) - (Jxy * Jyx);

			pNormalOut[4 * n + 3] = (jacobian < 0) ? 1.0f : 0.0f;

			// Preparation for IFFT execution
			m_FFTin[1][n][0] = -1 * m_kVectors[n].x * m_FFTin[0][n][1];
			m_FFTin[1][n][1] = m_kVectors[n].x * m_FFTin[0][n][0];

			m_FFTin[2][n][0] = -1 * m_kVectors[n].y * m_FFTin[0][n][1];
			m_FFTin[2][n][1] = m_kVectors[n].y * m_FFTin[0][n][0];
		}
	}

	// IFFT execution
	fftwf_execute(m_plan[1]);
	fftwf_execute(m_plan[2]);

	for (n = 0; n < m_height * m_width; ++n)
	{
		pNormalOut[4 * n + 0] = (m_FFTout[1][n][0]);		//X
		pNormalOut[4 * n + 1] = 2500;						//Y
		pNormalOut[4 * n + 2] = (m_FFTout[2][n][0]);		//Z
	}
}

void FFTWrapper::Fill_Normals_Central_Diff(const float& choppy, const float &heightAdj, const float& foamInt)
{
	int xNext, zNext, n, nInd;
	float s11, s21, s12;
	float Jxx, Jyy, Jxy, Jyx, jacobian;
	v3 va, vb, normals;

	float intensity = 1 / (m_height / (1 + foamInt));
	intensity = (foamInt == 0) ? 0 : intensity;

	// Calculate the Jacobian along with the normals
	// with central difference

	for (int j(0); j < m_height; ++j)
	{
		for (int i(0); i < m_width; ++i)
		{
			xNext = (i == m_width - 1) ? xNext = 0 : xNext = i + 1;
			zNext = (j == m_height - 1) ? zNext = 0 : zNext = j + 1;

			n = j * m_height + i;
			s11 = heightAdj * pImageOut[4 * n + 1];


			nInd = j * m_height + xNext;
			s21 = heightAdj * pImageOut[4 * nInd + 1];
			Jxx = 1 + choppy * (-m_FFTout[1][nInd][0] + m_FFTout[1][n][0]) * intensity;
			Jxy = choppy * (-m_FFTout[2][nInd][0] + m_FFTout[2][n][0]) * intensity;


			nInd = zNext * m_height + i;
			s12 = heightAdj * pImageOut[4 * nInd + 1];
			Jyy = 1 + choppy * (-m_FFTout[2][nInd][0] + m_FFTout[2][n][0]) * intensity;
			Jyx = choppy * (-m_FFTout[1][nInd][0] + m_FFTout[1][n][0]) * intensity;

			va = v3(2.0f, 0.0f, s21 - s11);
			va.Normalize();

			vb = v3(0.0f, 2.0f, s12 - s11);
			vb.Normalize();

			normals = va.Cross(vb);	
			jacobian = (Jxx * Jyy) - (Jxy * Jyx);

			pNormalOut[4 * n + 0] = normals.x;	//X
			pNormalOut[4 * n + 1] = normals.z;	//Y (inverted axis)
			pNormalOut[4 * n + 2] = normals.y;	//Z
			pNormalOut[4 * n + 3] = (jacobian < 0) ? 1.0f : 0.0f;
		}
	}
}


void FFTWrapper::Generate_Heightmap()
{
	// Initialisation of the model
	Fill_K_Vectors();
	Fill_h0tilde();

#if defined(CPU_NORM_CD) | defined(CPU_NORM_FFT)
	Precalculate_Sinusoids();
#endif // CPU_NORM_CD | CPU_NORM_FFT

}

void FFTWrapper::IFFT_Thread()
{
	// Parallel IFFT execution for the calculation 
	// of the heightmap and the horizontal displacement 

	fftwf_execute(m_plan[0]);
	fftwf_execute(m_plan[1]);
	fftwf_execute(m_plan[2]);
}
