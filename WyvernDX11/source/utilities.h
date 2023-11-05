#pragma once

#include <string_view>
#include <shobjidl.h> 
#include <sstream>
#include <string>
#include <algorithm>
#include "glm.h"

#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))

struct ldiLine {
	vec3 origin;
	vec3 direction;
};

struct ldiLineSegment {
	vec3 a;
	vec3 b;
};

struct ldiPlane {
	vec3 point;
	vec3 normal;
};

//----------------------------------------------------------------------------------------------------
// Planes.
//----------------------------------------------------------------------------------------------------
mat4 planeGetBasis(ldiPlane Plane) {
	vec3 up = vec3Up;

	if (up == Plane.normal) {
		up = vec3Right;
	}

	vec3 side = glm::cross(Plane.normal, up);
	side = glm::normalize(side);
	up = glm::cross(Plane.normal, side);
	up = glm::normalize(up);

	mat4 result = glm::identity<mat4>();
	result[0] = vec4(side, 0.0f);
	result[1] = vec4(up, 0.0f);
	result[2] = vec4(Plane.normal, 0.0f);
	result[3] = vec4(Plane.point, 1.0f);

	return result;
}

bool getPointAtIntersectionOfPlanes(ldiPlane A, ldiPlane B, ldiPlane C, vec3* OutPoint) {
	vec3 m1 = vec3(A.normal.x, B.normal.x, C.normal.x);
	vec3 m2 = vec3(A.normal.y, B.normal.y, C.normal.y);
	vec3 m3 = vec3(A.normal.z, B.normal.z, C.normal.z);

	float distA = glm::dot(A.normal, A.point);
	float distB = glm::dot(B.normal, B.point);
	float distC = glm::dot(C.normal, C.point);

	vec3 d = vec3(distA, distB, distC);

	vec3 u = glm::cross(m2, m3);
	vec3 v = glm::cross(m1, d);

	float denom = glm::dot(m1, u);

	if (abs(denom) < std::numeric_limits<float>::epsilon()) {
		return false;
	}

	*OutPoint = vec3(glm::dot(d, u) / denom, glm::dot(m3, v) / denom, -glm::dot(m2, v) / denom);

	return true;
}

vec3 projectPointToPlane(vec3 Point, ldiPlane Plane) {
	vec3 delta = Point - Plane.point;
	float dist = glm::dot(delta, Plane.normal);
	return Point - Plane.normal * dist;
}

bool getRayPlaneIntersection(ldiLine Ray, ldiPlane Plane, vec3& Point) {
	float denom = glm::dot(Plane.normal, Ray.direction);
	
	if (denom > std::numeric_limits<float>::epsilon()) {
		vec3 p0l0 = Plane.point - Ray.origin;
		float t = glm::dot(p0l0, Plane.normal) / denom;

		if (t >= 0) {
			Point = Ray.origin + Ray.direction * t;

			return true;
		}
	}

	return false;
}

//----------------------------------------------------------------------------------------------------
// Strings.
//----------------------------------------------------------------------------------------------------
static bool endsWith(std::string_view str, std::string_view suffix) {
	return str.size() >= suffix.size() && 0 == str.compare(str.size() - suffix.size(), suffix.size(), suffix);
}

static bool startsWith(std::string_view str, std::string_view prefix) {
	return str.size() >= prefix.size() && 0 == str.compare(0, prefix.size(), prefix);
}

void convertStrToWide(const char* Str, wchar_t* WideStr) {
	size_t size = strlen(Str) + 1;
	size_t outSize;
	mbstowcs_s(&outSize, WideStr, size, Str, size - 1);
}

//----------------------------------------------------------------------------------------------------
// Files.
//----------------------------------------------------------------------------------------------------
std::vector<std::string> listAllFilesInDirectory(const std::string& Path) {
	std::vector<std::string> result;

	try {
		for (const auto& entry : std::filesystem::directory_iterator(Path)) {
			result.push_back(entry.path().string());
		}	
	} catch (const std::exception& e) {
		std::cout << "Error: " << e.what() << "\n";
	}

	return result;
}

bool removeAllFilesInDirectory(const std::string& Path) {
	std::filesystem::remove_all(Path);

	return true;
}

bool createDirectory(const std::string& Path) {
	return true;
}

bool copyFile(const std::string& Src, const std::string& Dst) {
	return std::filesystem::copy_file(Src, Dst, std::filesystem::copy_options::overwrite_existing);
}

int readFile(const std::string& Path, uint8_t** Data) {
	FILE* file;

	if (fopen_s(&file, Path.c_str(), "rb") != 0) {
		*Data = nullptr;
		return -1;
	}

	fseek(file, 0, SEEK_END);
	long fileLen = ftell(file);
	fseek(file, 0, SEEK_SET);
	uint8_t* fileBuffer = new uint8_t[fileLen];
	fread(fileBuffer, 1, fileLen, file);

	*Data = fileBuffer;
	
	return (int)fileLen;
}

bool showOpenFileDialog(HWND WindowHandle, const std::string& DefaultFolderPath, std::string& FilePath, const LPCWSTR ExtensionInfo, const LPCWSTR ExtensionFilter) {
	bool result = false;
	
	wchar_t defaultFolderWStr[1024];
	convertStrToWide(DefaultFolderPath.c_str(), defaultFolderWStr);

	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (SUCCEEDED(hr)) {
		IFileOpenDialog* pFileOpen;
		hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

		if (SUCCEEDED(hr)) {
			IShellItem* pDefaultFolderItem;
			hr = SHCreateItemFromParsingName(defaultFolderWStr, NULL, IID_IShellItem, reinterpret_cast<void**>(&pDefaultFolderItem));
			if (SUCCEEDED(hr)) {
				COMDLG_FILTERSPEC rgSpec[] = {
					{ ExtensionInfo, ExtensionFilter }
				};

				pFileOpen->SetFileTypes(1, rgSpec);
				hr = pFileOpen->SetDefaultFolder(pDefaultFolderItem);
				
				if (SUCCEEDED(hr)) {
					hr = pFileOpen->Show(WindowHandle);

					if (SUCCEEDED(hr)) {
						IShellItem* pItem;
						hr = pFileOpen->GetResult(&pItem);
						if (SUCCEEDED(hr)) {
							PWSTR pszFilePath;
							hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

							// Display the file name to the user.
							if (SUCCEEDED(hr)) {
								std::wstring wstr = pszFilePath;
								FilePath = std::string(wstr.begin(), wstr.end());
								CoTaskMemFree(pszFilePath);
								result = true;
							}
							pItem->Release();
						}
					}
				}
				pDefaultFolderItem->Release();
			}
			pFileOpen->Release();
		}
		CoUninitialize();
	}

	return result;
}

bool showSaveFileDialog(HWND WindowHandle, const std::string& DefaultFolderPath, std::string& FilePath, const LPCWSTR ExtensionInfo, const LPCWSTR ExtensionFilter, const LPCWSTR DefaultExtension) {
	bool result = false;

	wchar_t defaultFolderWStr[1024];
	convertStrToWide(DefaultFolderPath.c_str(), defaultFolderWStr);

	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (SUCCEEDED(hr)) {
		IFileSaveDialog* pFileSave;
		hr = CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_ALL, IID_IFileSaveDialog, reinterpret_cast<void**>(&pFileSave));

		if (SUCCEEDED(hr)) {
			IShellItem* pDefaultFolderItem;
			hr = SHCreateItemFromParsingName(defaultFolderWStr, NULL, IID_IShellItem, reinterpret_cast<void**>(&pDefaultFolderItem));
			if (SUCCEEDED(hr)) {
				COMDLG_FILTERSPEC rgSpec[] = {
					{ ExtensionInfo, ExtensionFilter }
				};

				pFileSave->SetFileTypes(1, rgSpec);
				pFileSave->SetDefaultExtension(DefaultExtension);
				hr = pFileSave->SetDefaultFolder(pDefaultFolderItem);

				if (SUCCEEDED(hr)) {
					hr = pFileSave->Show(WindowHandle);

					if (SUCCEEDED(hr)) {
						IShellItem* pItem;
						hr = pFileSave->GetResult(&pItem);
						if (SUCCEEDED(hr)) {
							PWSTR pszFilePath;
							hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

							// Display the file name to the user.
							if (SUCCEEDED(hr)) {
								std::wstring wstr = pszFilePath;
								FilePath = std::string(wstr.begin(), wstr.end());
								CoTaskMemFree(pszFilePath);
								result = true;
							}
							pItem->Release();
						}
					}
				}
				pDefaultFolderItem->Release();
			}
			pFileSave->Release();
		}
		CoUninitialize();
	}

	return result;
}

//----------------------------------------------------------------------------------------------------
// Color.
//----------------------------------------------------------------------------------------------------
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

vec3 HSVtoRGB(float H, float S, float V) {
	/*if (H > 360 || H < 0 || S>100 || S < 0 || V>100 || V < 0) {
		cout << "The givem HSV values are not in valid range" << endl;
		return;
	}*/

	float s = S / 100;
	float v = V / 100;
	float C = s * v;
	float X = C * (1 - abs(fmod(H / 60.0, 2) - 1));
	float m = v - C;
	float r, g, b;

	if (H >= 0 && H < 60) {
		r = C, g = X, b = 0;
	} else if (H >= 60 && H < 120) {
		r = X, g = C, b = 0;
	} else if (H >= 120 && H < 180) {
		r = 0, g = C, b = X;
	} else if (H >= 180 && H < 240) {
		r = 0, g = X, b = C;
	} else if (H >= 240 && H < 300) {
		r = X, g = 0, b = C;
	} else {
		r = C, g = 0, b = X;
	}
	
	return vec3((r + m), (g + m), (b + m));
}

vec3 getRandomColorHighSaturation() {
	float hue = ((float)rand() / (float)RAND_MAX) * 359;
	float sat = ((float)rand() / (float)RAND_MAX) * 10 + 89;
	float val = ((float)rand() / (float)RAND_MAX) * 10 + 89;

	return HSVtoRGB(hue, sat, val);
}

vec3 getRandomColor() {
	vec3 result;
	result.x = (rand() % 255) / 255.0f;
	result.y = (rand() % 255) / 255.0f;
	result.z = (rand() % 255) / 255.0f;

	return result;
}

//----------------------------------------------------------------------------------------------------
// Conversions.
//----------------------------------------------------------------------------------------------------
inline ImVec2 toImVec2(vec2 A) {
	return ImVec2(A.x, A.y);
}

inline vec2 toVec2(ImVec2 A) {
	return vec2(A.x, A.y);
}

inline vec2 toVec2(cv::Point2f A) {
	return vec2(A.x, A.y);
}

inline cv::Point2f toPoint2f(vec2 A) {
	return cv::Point2f(A.x, A.y);
}

inline cv::Point3f toPoint3f(vec3 A) {
	return cv::Point3f(A.x, A.y, A.z);
}

inline vec3 toVec3(cv::Point3f A) {
	return vec3(A.x, A.y, A.z);
}

inline vec3d toVec3(cv::Point3d A) {
	return vec3d(A.x, A.y, A.z);
}

inline std::string GetStr(vec3 V) {
	std::stringstream str;

	str << V.x << ", " << V.y << ", " << V.z;

	return str.str();
}

std::string GetStr(mat4* Mat) {
	std::stringstream str;

	str << (*Mat)[0][0] << " " << (*Mat)[0][1] << " " << (*Mat)[0][2] << " " << (*Mat)[0][3] << "\n";
	str << (*Mat)[1][0] << " " << (*Mat)[1][1] << " " << (*Mat)[1][2] << " " << (*Mat)[1][3] << "\n";
	str << (*Mat)[2][0] << " " << (*Mat)[2][1] << " " << (*Mat)[2][2] << " " << (*Mat)[2][3] << "\n";
	str << (*Mat)[3][0] << " " << (*Mat)[3][1] << " " << (*Mat)[3][2] << " " << (*Mat)[3][3] << "\n";

	return str.str();
}

mat4 convertOpenCvTransMatToGlmMat(cv::Mat& TransMat) {
	// NOTE: Just a transpose.
	mat4 result = glm::identity<mat4>();

	for (int iY = 0; iY < 3; ++iY) {
		for (int iX = 0; iX < 4; ++iX) {
			result[iX][iY] = TransMat.at<double>(iY, iX);
		}
	}

	return result;
}

mat4 convertRvecTvec(cv::Mat R, cv::Mat T) {
	mat4 result;

	cv::Mat cvRotMat = cv::Mat::zeros(3, 3, CV_64F);
	cv::Rodrigues(R, cvRotMat);

	result[0][0] = cvRotMat.at<double>(0, 0);
	result[0][1] = cvRotMat.at<double>(1, 0);
	result[0][2] = cvRotMat.at<double>(2, 0);
	result[1][0] = cvRotMat.at<double>(0, 1);
	result[1][1] = cvRotMat.at<double>(1, 1);
	result[1][2] = cvRotMat.at<double>(2, 1);
	result[2][0] = cvRotMat.at<double>(0, 2);
	result[2][1] = cvRotMat.at<double>(1, 2);
	result[2][2] = cvRotMat.at<double>(2, 2);
	result[3][0] = T.at<double>(0);
	result[3][1] = T.at<double>(1);
	result[3][2] = T.at<double>(2);
	result[3][3] = 1.0;

	return result;
}

// Takes single matrix of R and T combined.
cv::Mat convertRvecTvec(cv::Mat RT) {
	cv::Mat result = cv::Mat::zeros(4, 4, CV_64F);

	cv::Mat cvRotMat = cv::Mat::zeros(3, 3, CV_64F);
	cv::Mat r = cv::Mat::zeros(1, 3, CV_64F);
	r.at<double>(0) = RT.at<double>(0);
	r.at<double>(1) = RT.at<double>(1);
	r.at<double>(2) = RT.at<double>(2);
	cv::Rodrigues(r, cvRotMat);

	result.at<double>(0, 0) = cvRotMat.at<double>(0, 0);
	result.at<double>(1, 0) = cvRotMat.at<double>(1, 0);
	result.at<double>(2, 0) = cvRotMat.at<double>(2, 0);
	result.at<double>(0, 1) = cvRotMat.at<double>(0, 1);
	result.at<double>(1, 1) = cvRotMat.at<double>(1, 1);
	result.at<double>(2, 1) = cvRotMat.at<double>(2, 1);
	result.at<double>(0, 2) = cvRotMat.at<double>(0, 2);
	result.at<double>(1, 2) = cvRotMat.at<double>(1, 2);
	result.at<double>(2, 2) = cvRotMat.at<double>(2, 2);
	result.at<double>(0, 3) = RT.at<double>(3);
	result.at<double>(1, 3) = RT.at<double>(4);
	result.at<double>(2, 3) = RT.at<double>(5);
	result.at<double>(3, 3) = 1.0;

	return result;
}

cv::Mat convertTransformToRT(cv::Mat Trans) {
	cv::Mat result = cv::Mat::zeros(1, 6, CV_64F);

	cv::Mat cvRotMat = cv::Mat::zeros(3, 3, CV_64F);
	cvRotMat.at<double>(0, 0) = Trans.at<double>(0, 0);
	cvRotMat.at<double>(1, 0) = Trans.at<double>(1, 0);
	cvRotMat.at<double>(2, 0) = Trans.at<double>(2, 0);
	cvRotMat.at<double>(0, 1) = Trans.at<double>(0, 1);
	cvRotMat.at<double>(1, 1) = Trans.at<double>(1, 1);
	cvRotMat.at<double>(2, 1) = Trans.at<double>(2, 1);
	cvRotMat.at<double>(0, 2) = Trans.at<double>(0, 2);
	cvRotMat.at<double>(1, 2) = Trans.at<double>(1, 2);
	cvRotMat.at<double>(2, 2) = Trans.at<double>(2, 2);

	cv::Mat r = cv::Mat::zeros(1, 3, CV_64F);
	cv::Rodrigues(cvRotMat, r);

	result.at<double>(0) = r.at<double>(0);
	result.at<double>(1) = r.at<double>(1);
	result.at<double>(2) = r.at<double>(2);
	result.at<double>(3) = Trans.at<double>(0, 3);
	result.at<double>(4) = Trans.at<double>(1, 3);
	result.at<double>(5) = Trans.at<double>(2, 3);

	return result;
}