#pragma once

#include <string_view>
#include <shobjidl.h> 
#include <sstream>
#include <string>

static bool endsWith(std::string_view str, std::string_view suffix) {
	return str.size() >= suffix.size() && 0 == str.compare(str.size() - suffix.size(), suffix.size(), suffix);
}

static bool startsWith(std::string_view str, std::string_view prefix) {
	return str.size() >= prefix.size() && 0 == str.compare(0, prefix.size(), prefix);
}

std::vector<std::string> listAllFilesInDirectory(std::string Path) {
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

bool removeAllFilesInDirectory(std::string Path) {
	std::filesystem::remove_all(Path);

	return true;
}

bool createDirectory(std::string Path) {
	return true;
}

bool copyFile(std::string Src, std::string Dst) {
	return std::filesystem::copy_file(Src, Dst, std::filesystem::copy_options::overwrite_existing);
}

void convertStrToWide(const char* Str, wchar_t* WideStr) {
	size_t size = strlen(Str) + 1;
	size_t outSize;
	mbstowcs_s(&outSize, WideStr, size, Str, size - 1);
}

bool showOpenFileDialog(HWND WindowHandle, std::string& DefaultFolderPath, std::string& FilePath, const LPCWSTR ExtensionInfo, const LPCWSTR ExtensionFilter) {
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

#include "glm.h"

std::string GetMat4DebugString(mat4* Mat) {
	std::stringstream str;
	
	str << (*Mat)[0][0] << " " << (*Mat)[0][1] << " " << (*Mat)[0][2] << " " << (*Mat)[0][3] << "\n";
	str << (*Mat)[1][0] << " " << (*Mat)[1][1] << " " << (*Mat)[1][2] << " " << (*Mat)[1][3] << "\n";
	str << (*Mat)[2][0] << " " << (*Mat)[2][1] << " " << (*Mat)[2][2] << " " << (*Mat)[2][3] << "\n";
	str << (*Mat)[3][0] << " " << (*Mat)[3][1] << " " << (*Mat)[3][2] << " " << (*Mat)[3][3] << "\n";

	return str.str();
}