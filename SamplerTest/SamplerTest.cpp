#include <stdint.h>
#include <stdlib.h>
#include <iostream>
#include <chrono>

#define _USE_MATH_DEFINES
#include <math.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <windows.h>

//---------------------------------------------------------------------------------------------------------------
// Profile timer utility.
//---------------------------------------------------------------------------------------------------------------
using namespace std::chrono;

typedef high_resolution_clock Clock;

double getProfileTime(steady_clock::time_point A, steady_clock::time_point B) {
	return duration_cast<microseconds>(B - A).count() / 1000.0;
}

//---------------------------------------------------------------------------------------------------------------
// Image space helper functions.
//---------------------------------------------------------------------------------------------------------------
float GammaToLinear(float In) {
	float linearRGBLo = In / 12.92f;
	float linearRGBHi = pow(max(abs((In + 0.055f) / 1.055f), 1.192092896e-07f), 2.4f);

	return (In <= 0.04045) ? linearRGBLo : linearRGBHi;
}

float LinearToGamma(float In) {
	float sRGBLo = In * 12.92f;
	float sRGBHi = (pow(max(abs(In), 1.192092896e-07f), 1.0f / 2.4f) * 1.055f) - 0.055f;
	return (In <= 0.0031308f) ? sRGBLo : sRGBHi;
}

//---------------------------------------------------------------------------------------------------------------
// Sampler helpers.
//---------------------------------------------------------------------------------------------------------------
struct SamplePoint {
	float X;
	float Y;
	float Value;
	float Scale;
	float Radius;
	int nextSampleId;
};

// Get intensity value from source texture at x,y, 0.0 to 1.0.
float GetSourceValue(float* Data, int Width, int Height, float X, float Y) {
	int pX = (int)(X * Width);
	int pY = (int)(Y * Height);
	int idx = pY * Width + pX;

	return Data[idx];
}

int clamp(int Value, int Min, int Max) {
	return min(max(Value, Min), Max);
}

//---------------------------------------------------------------------------------------------------------------
// Output image.
//---------------------------------------------------------------------------------------------------------------
struct OutputImage {
	int PixelWidth;
	int PixelHeight;
	float Scale;
	uint8_t* Data;
};

void InitImage(OutputImage* Image, int Width, int Height, float Scale) {
	Image->PixelWidth = Width;
	Image->PixelHeight = Height;
	Image->Scale = Scale;
	Image->Data = new uint8_t[Width * Height * 3];
}

void ClearImage(OutputImage* Image) {
	for (int i = 0; i < Image->PixelHeight * Image->PixelWidth; ++i) {
		Image->Data[i * 3 + 0] = 255;
		Image->Data[i * 3 + 1] = 255;
		Image->Data[i * 3 + 2] = 255;
	}
}

inline void SetPixelImage(OutputImage* Image, int X, int Y, uint8_t R, uint8_t G, uint8_t B) {
	int pixelIdx = (Y * Image->PixelWidth + X) * 3;
	Image->Data[pixelIdx + 0] = R;
	Image->Data[pixelIdx + 1] = G;
	Image->Data[pixelIdx + 2] = B;
}

void DrawCircle(OutputImage* Image, float X, float Y, float Radius, float Value = 0.0f) {
	float tX = X * Image->Scale;
	float tY = Y * Image->Scale;
	float tR = Radius * Image->Scale;

	int gSx = (int)((tX - tR));
	int gEx = (int)((tX + tR));
	int gSy = (int)((tY - tR));
	int gEy = (int)((tY + tR));

	gSx = clamp(gSx, 0, Image->PixelWidth - 1);
	gEx = clamp(gEx, 0, Image->PixelWidth - 1);
	gSy = clamp(gSy, 0, Image->PixelHeight - 1);
	gEy = clamp(gEy, 0, Image->PixelHeight - 1);

	float radiusSqr = tR * tR;

	uint8_t color = (uint8_t)(Value * 255.0f);

	for (int iY = gSy; iY <= gEy; ++iY) {
		for (int iX = gSx; iX <= gEx; ++iX) {
			float dX = tX - iX;
			float dY = tY - iY;
			float distSqr = dX * dX + dY * dY;

			if (distSqr <= radiusSqr) {
				SetPixelImage(Image, iX, iY, color, color, color);
			}
		}
	}

	// SetPixelImage(Image, tX, tY, 255, 0, 0);
}

//---------------------------------------------------------------------------------------------------------------
// Main application.
//---------------------------------------------------------------------------------------------------------------
int main()
{
	std::cout << "Started sampler.\n";
	
	char strBuffer[1024];
	int value = GetCurrentDirectory(1024, strBuffer);
	std::cout << "Working directory: " << strBuffer << "\n";

	//---------------------------------------------------------------------------------------------------------------
	// Load image.
	//---------------------------------------------------------------------------------------------------------------
	auto t0 = Clock::now();

	int sourceWidth, sourceHeight, sourceChannels;
	//uint8_t* sourcePixels = stbi_load("images\\dergn.png", &sourceWidth, &sourceHeight, &sourceChannels, 0);
	uint8_t* sourcePixels = stbi_load("images\\dergn2.png", &sourceWidth, &sourceHeight, &sourceChannels, 0);
	//uint8_t* sourcePixels = stbi_load("images\\char_render_sat_smaller.png", &sourceWidth, &sourceHeight, &sourceChannels, 0);
	//uint8_t* sourcePixels = stbi_load("images\\imM.png", &sourceWidth, &sourceHeight, &sourceChannels, 0);

	auto t1 = Clock::now();
	std::cout << "Image loaded: " << getProfileTime(t0, t1) << " ms\n";

	std::cout << "Image stats: " << sourceWidth << ", " << sourceHeight << " " << sourceChannels << "\n";

	//---------------------------------------------------------------------------------------------------------------
	// Convert image to greyscale
	//---------------------------------------------------------------------------------------------------------------
	int sourceTotalPixels = sourceWidth * sourceHeight;

	float* sourceIntensity = new float[sourceTotalPixels];

	t0 = Clock::now();

	for (int i = 0; i < sourceTotalPixels; ++i) {
		uint8_t r = sourcePixels[i * sourceChannels + 0];
		//uint8_t g = sourcePixels[i * sourceChannels + 1];
		//uint8_t b = sourcePixels[i * sourceChannels + 2];

		float lum = r / 255.0f;// (r * 0.2126f + g * 0.7152f + b * 0.0722f) / 255.0f;
		//float lum = (r * 0.2126f + g * 0.7152f + b * 0.0722f) / 255.0f;

		sourceIntensity[i] = GammaToLinear(lum);
		//sourceIntensity[i] = lum;
	}

	t1 = Clock::now();
	std::cout << "Greyscale: " << getProfileTime(t0, t1) << " ms\n";

	//---------------------------------------------------------------------------------------------------------------
	// Poisson disk sampler - generate sample pool.
	//---------------------------------------------------------------------------------------------------------------
	t0 = Clock::now();

	float samplesPerPixel = 8.0f;
	float sampleScale = ((sourceWidth / 10.0f) / sourceWidth) / samplesPerPixel;
	float singlePixelScale = sampleScale * 8;

	int samplesWidth = (int)(sourceWidth * samplesPerPixel);
	int samplesHeight = (int)(sourceHeight * samplesPerPixel);

	SamplePoint* candidateList = new SamplePoint[samplesWidth * samplesHeight];
	int candidateCount = 0;

	for (int iY = 0; iY < samplesHeight; ++iY) {
		for (int iX = 0; iX < samplesWidth; ++iX) {
			float sX = (float)iX / samplesWidth;
			float sY = (float)iY / samplesHeight;
			float value = GetSourceValue(sourceIntensity, sourceWidth, sourceHeight, sX, sY);

			if (value >= 0.999f) {
				continue;
			}

			SamplePoint* s = &candidateList[candidateCount++];
			s->X = iX * sampleScale;
			s->Y = iY * sampleScale;
			s->Value = value;

			float radiusMul = 2.4f;

			float radius = 1;

			float minDotSize = 0.2f;
			float minDotInv = 1.0 - minDotSize;

			if (value > minDotInv) {
				s->Value = minDotInv;

				float area = 1.0f / (1.0f - (value - minDotInv)) * 3.14f;
				radius = sqrt(area / M_PI);
				radius = pow(radius, 2.2f);
			}

			radius *= radiusMul;
			
			s->Scale = singlePixelScale * 0.5f * 1.4f;
			s->Radius = singlePixelScale * 0.5f * radius;
		}
	}

	t1 = Clock::now();
	std::cout << "Generate sample pool: " << getProfileTime(t0, t1) << " ms\n";
	std::cout << "Sample count: " << candidateCount << "\n";

	//---------------------------------------------------------------------------------------------------------------
	// Create random order for sample selection.
	//---------------------------------------------------------------------------------------------------------------
	t0 = Clock::now();

	int* orderList = new int[candidateCount];
	for (int i = 0; i < candidateCount; ++i) {
		orderList[i] = i;
	}

	// Randomize pairs
	for (int j = 0; j < candidateCount; ++j) {
		int targetSlot = rand() % candidateCount;

		int tmp = orderList[targetSlot];
		orderList[targetSlot] = orderList[j];
		orderList[j] = tmp;
	}

	t1 = Clock::now();
	std::cout << "Generate random sample order: " << getProfileTime(t0, t1) << " ms\n";

	//---------------------------------------------------------------------------------------------------------------
	// Build spatial table.
	//---------------------------------------------------------------------------------------------------------------
	float gridCellSize = 5 * singlePixelScale;
	float gridPotentialWidth = sampleScale * samplesWidth;
	float gridPotentialHeight = sampleScale * samplesHeight;
	int gridCellsX = (int)ceil(gridPotentialWidth / gridCellSize);
	int gridCellsY = (int)ceil(gridPotentialHeight / gridCellSize);
	int gridTotalCells = gridCellsX * gridCellsY;

	std::cout << "Grid size: " << gridCellsX << ", " << gridCellsY << "(" << gridCellSize << ") Total: " << gridTotalCells << "\n";

	int* gridCells = new int[gridCellsX * gridCellsY];

	for (int i = 0; i < gridCellsX * gridCellsY; ++i) {
		gridCells[i] = -1;
	}

	//---------------------------------------------------------------------------------------------------------------
	// Add samples from pool.
	//---------------------------------------------------------------------------------------------------------------
	t0 = Clock::now();

	int* pointList = new int[candidateCount];
	int pointCount = 0;

	for (int i = 0; i < candidateCount; ++i) {
		int candidateId = orderList[i];

		SamplePoint* candidate = &candidateList[candidateId];
		bool found = false;

		float halfGcs = candidate->Radius;

		int gSx = (int)((candidate->X - halfGcs) / gridCellSize);
		int gEx = (int)((candidate->X + halfGcs) / gridCellSize);
		int gSy = (int)((candidate->Y - halfGcs) / gridCellSize);
		int gEy = (int)((candidate->Y + halfGcs) / gridCellSize);

		gSx = clamp(gSx, 0, gridCellsX - 1);
		gEx = clamp(gEx, 0, gridCellsX - 1);
		gSy = clamp(gSy, 0, gridCellsY - 1);
		gEy = clamp(gEy, 0, gridCellsY - 1);

		for (int iY = gSy; iY <= gEy; ++iY) {
			for (int iX = gSx; iX <= gEx; ++iX) {

				int targetId = gridCells[iY * gridCellsX + iX];

				while (targetId != -1) {
					SamplePoint* target = &candidateList[targetId];

					float dX = candidate->X - target->X;
					float dY = candidate->Y - target->Y;
					float sqrDist = dX * dX + dY * dY;

					if (sqrDist <= (candidate->Radius * candidate->Radius)) {
						found = true;
						break;
					}

					targetId = target->nextSampleId;
				}

				if (found == true) {
					break;
				}
			}

			if (found == true) {
				break;
			}
		}

		if (!found) {
			pointList[pointCount++] = candidateId;

			int gX = (int)((candidate->X) / gridCellSize);
			int gY = (int)((candidate->Y) / gridCellSize);

			int tempId = gridCells[gY * gridCellsX + gX];
			candidate->nextSampleId = tempId;
			gridCells[gY * gridCellsX + gX] = candidateId;
		}
	}

	t1 = Clock::now();
	std::cout << "Generate poisson samples: " << getProfileTime(t0, t1) << " ms\n";
	std::cout << "Poisson sample count: " << pointCount << "\n";

	//---------------------------------------------------------------------------------------------------------------
	// Render sample image.
	//---------------------------------------------------------------------------------------------------------------
	t0 = Clock::now();

	OutputImage outputImage;
	InitImage(&outputImage, 8192, 8192, 100);
	ClearImage(&outputImage);

	for (int i = 0; i < pointCount; ++i) {
		SamplePoint* s = &candidateList[pointList[i]];

		// NOTE: Intensity based.
		//DrawCircle(&outputImage, s->X, s->Y, s->Scale, s->Value);
		// NOTE: Size based.
		DrawCircle(&outputImage, s->X, s->Y, s->Scale * (1.0 - s->Value), 0);
	}

	stbi_write_png("images\\output.png", outputImage.PixelWidth, outputImage.PixelHeight, 3, outputImage.Data, 1 * outputImage.PixelWidth * 3);
	t1 = Clock::now();
	std::cout << "Write image: " << getProfileTime(t0, t1) << " ms\n";

	//---------------------------------------------------------------------------------------------------------------
	// Render guassian sampled image.
	//---------------------------------------------------------------------------------------------------------------


	//---------------------------------------------------------------------------------------------------------------
	// Save intensity as image.
	//---------------------------------------------------------------------------------------------------------------
	/*t0 = Clock::now();

	uint8_t* outputPixels = new uint8_t[sourceTotalPixels];

	for (int i = 0; i < sourceTotalPixels; ++i) {
		outputPixels[i] = (uint8_t)(sourceIntensity[i] * 255);
	}
	
	stbi_write_png("images\\output.png", sourceWidth, sourceHeight, 1, outputPixels, 1 * sourceWidth);

	delete[] outputPixels;

	t1 = Clock::now();
	std::cout << "Write image: " << getProfileTime(t0, t1) << " ms\n";*/

	std::cout << "Done...\n";
}