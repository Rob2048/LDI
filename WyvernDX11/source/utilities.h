#pragma once

#include <string_view>

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