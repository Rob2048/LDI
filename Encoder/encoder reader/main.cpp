#include <iostream>
#include <stdint.h>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "CImg.h"

//------------------------------------------------------------------------------------------------------------------------
// Timer utility functions.
//------------------------------------------------------------------------------------------------------------------------
static int64_t timeFrequency;
static int64_t timeCounterStart;

void timerInit() {
	LARGE_INTEGER freq;
	LARGE_INTEGER counter;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&counter);
	timeCounterStart = counter.QuadPart;
	timeFrequency = freq.QuadPart;
}

double getTimeSeconds() {
	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);
	int64_t time = counter.QuadPart - timeCounterStart;
	double result = (double)time / ((double)timeFrequency);

	return result;
}

void blurKernel(uint8_t* Src, int32_t Width, int32_t Height, uint8_t* Dst) {
	int totalPixels = Width * Height;
	int kernelRadius = 3;

	for (int iX = 0; iX < Width; ++iX) {
		for (int iY = 0; iY < Height; ++iY) {
			uint8_t result = 0;

			for (int kX = iX - kernelRadius; kX < iX + kernelRadius; ++kX) {
				for (int kY = iY - kernelRadius; kY < iY + kernelRadius; ++kY) {
					if (kX >= 0 && kY >= 0 && kX < Width && kY < Height) {
						result += Src[kY * Width + kX];
					}
				}
			}

			if (result > 0) {
				result = 255;
			}

			Dst[iY * Width + iX] = result;
		}
	}
}

struct point {
	float x;
	float y;
};

point lerpLine(float x0, float y0, float x1, float y1, float t) {
	point result;

	result.x = (x1 - x0) * t + x0;
	result.y = (y1 - y0) * t + y0;

	return result;
}

int countPixels(uint8_t* Img, int ImgWidth, int ImgHeight, int X0, int Y0, int X1, int Y1, int Value) {
	if (X0 < 0) X0 = 0;
	if (Y0 < 0) Y0 = 0;
	if (X1 >= ImgWidth) X1 = ImgWidth - 1;
	if (Y1 >= ImgHeight) Y1 = ImgHeight - 1;

	int result = 0;

	for (int iY = Y0; iY <= Y1; ++iY) {
		for (int iX = X0; iX <= X1; ++iX) {
			int idx = iY * ImgWidth + iX;

			if (Img[idx] == Value) {
				++result;
			}
		}
	}

	return result;
}

void blobDetect(uint8_t* Src, int32_t Width, int32_t Height, uint8_t* DebugDst) {
	int totalPixels = Width * Height;

	// Debug image.
	cimg_library::CImg<unsigned char> img(Width, Height, 1, 3);
	unsigned char white[] = { 255,255, 255 };
	unsigned char cyan[] = { 0, 255, 255 };
	unsigned char black[] = { 0, 0, 0 };
	unsigned char blue[] = { 0, 0, 255 };
	img.draw_rectangle(0, 0, Width, Height, white, 1.0);
	
	//-----------------------------------------------------------------------------------------
	// Create horizontal run spans.
	//-----------------------------------------------------------------------------------------
	double t = getTimeSeconds();

	struct seg {
		int x0;
		int x1;
		int y;
		int group;
		uint8_t v;
	};

	struct segRow {
		int segStart;
		int segCount;
	};

	segRow* segRows = new segRow[Height];
	seg* segs = new seg[32000];
	int segCount = 0;
	uint8_t lastSeg = 0;
	
	for (int iY = 0; iY < Height; ++iY) {
		int idxY = iY * Width;

		lastSeg = Src[idxY];

		segRows[iY].segStart = segCount;
		segRows[iY].segCount = 0;

		segs[segCount].group = -1;
		segs[segCount].x0 = 0;
		segs[segCount].y = iY;
		segs[segCount].v = lastSeg;
		++segCount;
		++segRows[iY].segCount;

		for (int iX = 1; iX < Width; ++iX) {
			int idx = idxY + iX;

			if (Src[idx] != lastSeg) {
				lastSeg = Src[idx];

				segs[segCount - 1].x1 = iX - 1;

				segs[segCount].group = -1;
				segs[segCount].x0 = iX;
				segs[segCount].y = iY;
				segs[segCount].v = lastSeg;
				++segCount;
				++segRows[iY].segCount;
			}
		}

		segs[segCount - 1].x1 = Width - 1;
	}
	
	t = getTimeSeconds() - t;
	std::cout << "Span time: " << (t * 1000) << " ms\n";
	std::cout << "Seg count: " << segCount << "\n";

	//// Draw horizontal spans.
	//for (int i = 0; i < Height; ++i) {
	//	segRow* row = &segRows[i];

	//	for (int j = row->segStart; j < row->segStart + row->segCount; ++j) {
	//		seg* s = &segs[j];
	//		for (int k = s->x0; k <= s->x1; ++k) {
	//			int idx = i * Width + k;

	//			int lastGroup = j;
	//			DebugDst[(idx) * 3 + 0] = ((lastGroup + 1) * 15) % 255;
	//			DebugDst[(idx) * 3 + 1] = ((lastGroup + 1) * 561) % 255;
	//			DebugDst[(idx) * 3 + 2] = ((lastGroup + 1) * 12) % 255;
	//		}
	//	}
	//}

	//-----------------------------------------------------------------------------------------
	// Group touching horizontal segments.
	//-----------------------------------------------------------------------------------------
	t = getTimeSeconds();

	struct groupElement {
		int x0;
		int y0;
		int x1;
		int y1;
		int joinGroup;
		int v;
		int groupId;
	};

	groupElement* groupElements = new groupElement[32000];
	int groupElementCount = 0;
	int groupIdCount = 0;

	for (int i = 0; i < Height - 1; ++i) {
		segRow* rowA = &segRows[i];
		segRow* rowB = &segRows[i + 1];

		for (int j = 0; j < rowA->segCount; ++j) {
			seg* sA = &segs[rowA->segStart + j];

			// If segment doesn't have a group, then it needs a new group.
			if (sA->group == -1) {
				sA->group = groupElementCount;

				groupElements[groupElementCount].x0 = sA->x0;
				groupElements[groupElementCount].x1 = sA->x1;
				groupElements[groupElementCount].y0 = i;
				groupElements[groupElementCount].y1 = i;
				groupElements[groupElementCount].v = sA->v;
				groupElements[groupElementCount].joinGroup = -1;
				groupElements[groupElementCount].groupId = groupIdCount++;
				++groupElementCount;
			} else {
				//groupElements[s->group].y1 = i;
				//if (s->x0 < groupElements[s->group].x0) groupElements[s->group].x0 = s->x0;
				//if (s->x1 > groupElements[s->group].x1) groupElements[s->group].x1 = s->x1;
			}

			// Compare seg to next row.
			for (int k = 0; k < rowB->segCount; ++k) {
				seg* sB = &segs[rowB->segStart + k];

				// Early out for segs that are obviously passed this one.
				if (sB->x0 - 1 > sA->x1) {
					break;
				}

				if (sB->v == sA->v && sB->x0 <= sA->x1 + 1 && sB->x1 >= sA->x0 - 1) {
					if (sB->group == -1) {
						sB->group = sA->group;
					} else if (sA->group != sB->group) {
						//sA->group = sB->group;
						//groupElements[sB->group].joinGroup = sA->group;
						//std::cout << "Merge " << sB->group << " to " << sA->group << "\n";

						// Find all groups with id of A and convert to id of B;
						int oldId = groupElements[sB->group].groupId;
						int newId = groupElements[sA->group].groupId;

						for (int iG = 0; iG < groupElementCount; ++iG) {
							if (groupElements[iG].groupId == oldId) {
								groupElements[iG].groupId = newId;
							}
						}
					}
				}
			}
		}
	}

	t = getTimeSeconds() - t;
	std::cout << "Group spans time: " << (t * 1000) << " ms\n";
	std::cout << "Group IDs: " << groupIdCount << "\n";
	std::cout << "Group elements: " << groupElementCount << "\n";

	// Draw horizontal spans.
	for (int i = 0; i < Height; ++i) {
		segRow* row = &segRows[i];

		for (int j = row->segStart; j < row->segStart + row->segCount; ++j) {
			seg* s = &segs[j];

			if (s->v == 0) {
				continue;
			}

			int lastGroup = 100;

			if (s->group >= 0) {
				lastGroup = groupElements[s->group].groupId;
				//lastGroup = s->group;
			}

			for (int k = s->x0; k <= s->x1; ++k) {
				int idx = i * Width + k;
				//DebugDst[(idx) * 3 + 0] = ((lastGroup + 1) * 15) % 255;
				//DebugDst[(idx) * 3 + 1] = ((lastGroup + 1) * 561) % 255;
				//DebugDst[(idx) * 3 + 2] = ((lastGroup + 1) * 12) % 255;

				unsigned char color[] = { ((lastGroup + 1) * 15) % 255, ((lastGroup + 1) * 561) % 255, ((lastGroup + 1) * 12) % 255 };
				img.draw_point(k, i, color, 1.0);
			}
		}
	}

	//-----------------------------------------------------------------------------------------
	// Bound final groups.
	//-----------------------------------------------------------------------------------------
	t = getTimeSeconds();

	struct pixelGroupElement {
		int x0;
		int y0;
		int x1;
		int y1;
		int joinGroup;
		int v;
		int groupId;
		double avgX;
		double avgY;
		int pixelCount;
	};

	pixelGroupElement* pixelGroups = new pixelGroupElement[groupIdCount];

	memset(pixelGroups, 0, sizeof(pixelGroupElement) * groupIdCount);
	
	for (int i = 0; i < segCount; ++i) {
		seg* s = &segs[i];
		groupElement* g = &groupElements[s->group];
		pixelGroupElement* pG = &pixelGroups[g->groupId];

		// Setup group that hasn't been used before;
		if (pG->groupId == 0) {
			pG->groupId = 1;
			pG->x0 = INT_MAX;
			pG->x1 = 0;
			pG->y0 = INT_MAX;
			pG->y1 = 0;
		}

		pG->v = s->v;

		// Add segment info to group.
		if (s->x0 < pG->x0) pG->x0 = s->x0;
		if (s->x1 > pG->x1) pG->x1 = s->x1;
		if (s->y < pG->y0) pG->y0 = s->y;
		if (s->y > pG->y1) pG->y1 = s->y;
	}

	int activePixelGroups = 0;

	for (int i = 0; i < groupIdCount; ++i) {
		if (pixelGroups[i].groupId == 1) {
			++activePixelGroups;
		}
	}

	t = getTimeSeconds() - t;
	std::cout << "Bound groups time: " << (t * 1000) << " ms\n";
	std::cout << "Active pixel groups: " << activePixelGroups << "\n";

	//-----------------------------------------------------------------------------------------
	// Find blobs.
	//-----------------------------------------------------------------------------------------
	t = getTimeSeconds();

	for (int i = 0; i < groupIdCount; ++i) {
		pixelGroupElement* g = &pixelGroups[i];

		// Ignore 'white' regions.
		if (g->v == 0) {
			continue;
		}

		int w = g->x1 - g->x0;
		int h = g->y1 - g->y0;
		float ratio = (float)w / (float)h;

		// Find size/ratio bounds.
		if (w > 40 && w < 200) {
			if (h > 40 && h < 200) {
				if (ratio > 0.8 && ratio < 1.2) {
					g->v = 50;
				}
			}
		}

		// Find centroid and coverage.
		if (g->v == 50) {
			double avgX = 0;
			double avgY = 0;
			int pixelCount = 0;

			for (int j = 0; j < segCount; ++j) {
				seg* s = &segs[j];
				if (groupElements[s->group].groupId == i) {
					for (int iX = s->x0; iX <= s->x1; ++iX) {
						avgX += iX;
						avgY += s->y;
						++pixelCount;
					}
				}
			}

			g->avgX = avgX / pixelCount;
			g->avgY = avgY / pixelCount;
			g->pixelCount = pixelCount;

			//std::cout << "Average for pixel group: " << i << " " << pixelCount << "\n";

			if (g->pixelCount > 4500 && g->pixelCount < 6000) {
				if (g->avgX < 380 && g->avgX > 320) {
					g->v = 52;
				}
			}

			if (g->pixelCount > 5000 && g->pixelCount < 11000) {
				if (g->avgX < 700 && g->avgX > 630) {
					g->v = 53;
				}

				if (g->avgX < 1450 && g->avgX > 1400) {
					g->v = 54;
				}
			}
		}
	}

	t = getTimeSeconds() - t;
	std::cout << "Blob centroid time: " << (t * 1000) << " ms\n";

	//-----------------------------------------------------------------------------------------
	// Read bitfield
	//-----------------------------------------------------------------------------------------
	t = getTimeSeconds();

	struct degLine {
		float x0;
		float y0;
		float x1;
		float y1;
		int degree;
	};

	std::vector<degLine> degLines;

	for (int i = 0; i < groupIdCount; ++i) {
		pixelGroupElement* gA = &pixelGroups[i];

		if (gA->v == 53) {
			for (int j = 0; j < groupIdCount; ++j) {
				pixelGroupElement* gB = &pixelGroups[j];

				if (gB->v == 54) {
					if (gA->avgY < gB->avgY + 50 && gA->avgY > gB->avgY - 50) {
						// Match found.

						float x0 = gA->avgX;
						float y0 = gA->avgY;
						float x1 = gB->avgX;
						float y1 = gB->avgY;

						point eP = lerpLine(x0, y0, x1, y1, -1);
						img.draw_line(eP.x, eP.y, x1, y1, blue);

						int bitfield = 0;

						for (int k = 0; k < 9; ++k) {
							float t = 1.0 / 12.5 * k + 0.18;
							point p = lerpLine(x0, y0, x1, y1, t);
							
							float bX0 = p.x - 20;
							float bY0 = p.y - 20;
							float bX1 = p.x + 20;
							float bY1 = p.y + 20;

							img.draw_rectangle(bX0, bY0, bX1, bY1, black, 0.5);

							int pCount = countPixels(Src, Width, Height, bX0, bY0, bX1, bY1, 255);

							char buff[256];
							sprintf_s(buff, sizeof(buff), "%d", pCount);
							img.draw_text(bX0, bY1, buff, black, 0, 1, 16);

							if (pCount < 1000) {
								img.draw_circle(p.x, p.y, 5, black);
							} else {
								img.draw_circle(p.x, p.y, 5, white);
								bitfield |= 1 << (8 - k);
							}
						}

						char buff[256];
						sprintf_s(buff, sizeof(buff), "%d", bitfield);
						img.draw_text(x1 + 100, y1, buff, black, 0, 1, 24);

						degLine line;
						line.degree = bitfield;
						line.x0 = x0;
						line.y0 = y0;
						line.x1 = x1;
						line.y1 = y1;

						degLines.push_back(line);

						break;
					}
				}
			}
		}
	}

	t = getTimeSeconds() - t;
	std::cout << "Bitfield time: " << (t * 1000) << " ms\n";

	if (!degLines.empty()) {
		for (int i = 0; i < degLines.size() - 1; ++i) {
			degLine lA = degLines[i];
			degLine lB = degLines[i + 1];

			float diff = lB.y0 - lA.y0;

			char buff[256];
			sprintf_s(buff, sizeof(buff), "%d", (int)diff);
			img.draw_text(lA.x0, lA.y0, buff, black, 0, 1, 16);

		}
	}

	//-----------------------------------------------------------------------------------------
	// Debug output.
	//-----------------------------------------------------------------------------------------
	float micronsToPixels = 62.0 / 200.0;
	float pixelsToMicrons = 1.0 / micronsToPixels;

	// Position blobs.
	img.draw_line(320, 0, 320, Height, blue);
	img.draw_line(380, 0, 380, Height, blue);

	// Bitfield locator blobs A.
	img.draw_line(630, 0, 630, Height, blue);
	img.draw_line(700, 0, 700, Height, blue);

	// Bitfield locator blobs A.
	img.draw_line(1400, 0, 1400, Height, blue);
	img.draw_line(1450, 0, 1450, Height, blue);

	// Bitfield.
	for (int i = 0; i < 10; ++i) {
		int x = 770 + (200 * micronsToPixels) * i;
		img.draw_line(x, 0, x, Height, blue);
	}

	for (int i = 0; i < groupIdCount; ++i) {
		pixelGroupElement* g = &pixelGroups[i];

		if (g->v == 0 || g->v == 255) {
			continue;
		}
				
		int cR = 255;
		int cG = 0;
		int cB = 0;

		/*int w = g->x1 - g->x0;
		int h = g->y1 - g->y0;

		if (w > 40 && w < 200) {
			if (h > 40 && h < 200) {
				cR = 0;
				cG = 255;
			}
		}*/

		if (g->v == 52) {
			cR = 0;
			cG = 255;
			cB = 0;
		}

		if (g->v == 53) {
			cR = 0;
			cG = 255;
			cB = 255;
		}

		if (g->v == 54) {
			cR = 255;
			cG = 0;
			cB = 255;
		}

		char buff[256];
		sprintf_s(buff, sizeof(buff), "%d :: %d", g->v, g->pixelCount);
		img.draw_text(g->x1, g->y0, buff, black, 0, 1, 16);

		unsigned char color[] = { cR, cG, cB };

		for (int iX = g->x0; iX <= g->x1; ++iX) {
			for (int iY = g->y0; iY <= g->y1; ++iY) {

				if (!(iX > g->x0 && iX < g->x1 && iY > g->y0 && iY < g->y1)) {
					int idx = iY * Width + iX;
					img.draw_point(iX, iY, color);
				}
			}
		}

		//img.draw_point((int)g->avgX, (int)g->avgY, color);
		img.draw_circle((int)g->avgX, (int)g->avgY, 4, color);
	}

	for (int iY = 0; iY < Height; ++iY) {
		for (int iX = 0; iX < Width; ++iX) {
			int idx = iY * Width + iX;

			DebugDst[idx * 3 + 0] = img.data()[img.offset(iX, iY, 0, 0)];;
			DebugDst[idx * 3 + 1] = img.data()[img.offset(iX, iY, 0, 1)];;
			DebugDst[idx * 3 + 2] = img.data()[img.offset(iX, iY, 0, 2)];;
		}
	}
}

//------------------------------------------------------------------------------------------------------------------------
// Application entry.
//------------------------------------------------------------------------------------------------------------------------
int main() {
	std::cout << "Starting encoder reader...\n";
	timerInit();

	int inputW, inputH, inputD;
	//uint8_t* inputImg = stbi_load("captures/test_x_1080_b.jpg", &inputW, &inputH, &inputD, 0);
	uint8_t* inputImg = stbi_load("captures/sim_b.jpg", &inputW, &inputH, &inputD, 0);
	std::cout << "Input img: " << inputW << " " << inputH << " " << inputD << "\n";

	int totalPixels = inputW * inputH;

	uint8_t* binaryData = new uint8_t[totalPixels];
	uint8_t* debugImg = new uint8_t[inputW * inputH * 3];
	uint8_t* tempData = new uint8_t[totalPixels];

	//------------------------------------------------------------------------------------------------------------------------
	// Greyscale & levels.
	//------------------------------------------------------------------------------------------------------------------------
	double t = getTimeSeconds();

	for (int i = 0; i < totalPixels; ++i) {
		uint8_t r = inputImg[i * inputD + 0];
		uint8_t g = inputImg[i * inputD + 1];
		uint8_t b = inputImg[i * inputD + 2];
		float y = (float)(0.2126 * r + 0.7152 * g + 0.0722 * b);
		uint8_t p = (uint8_t)y;

		//uint8_t p = inputImg[i * inputD];

		if (p > 60) {
			p = 0;
		} else {
			p = 255;
		}

		binaryData[i] = p;
	}

	t = getTimeSeconds() - t;
	std::cout << "Threshold time: " << (t * 1000) << " ms\n";

	t = getTimeSeconds();
	//blurKernel(binaryData, inputW, inputH, tempData);
	//memcpy(binaryData, tempData, totalPixels);
	t = getTimeSeconds() - t;
	std::cout << "Blur time: " << (t * 1000) << " ms\n";

	blobDetect(binaryData, inputW, inputH, debugImg);

	stbi_write_png("output/processed.png", inputW, inputH, inputD, debugImg, inputW * inputD);
}