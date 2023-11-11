#include "loader.hpp"

std::string loader::findCurrentDir() {
	std::string currentDirectory = std::filesystem::current_path().string();
	std::replace(currentDirectory.begin(), currentDirectory.end(), '\\', '/');
	currentDirectory.append("/");

	return currentDirectory;
}

const std::string loader::ReadFile(std::string FileName) {
	std::ostringstream sstream;
	std::ifstream File(FileName);
	sstream << File.rdbuf();
	const std::string FileData(sstream.str());

	return FileData;
}

bool loader::load() {
	std::string sceneDirectory = findCurrentDir();
	sceneDirectory.append("test.scene");
	std::string sceneContents = ReadFile(sceneDirectory);

	std::vector<int> numDigits;
	bool numIsNeg = false;
	for (int i = 0; i < sceneContents.length(); i++) {
		int charID = (int)sceneContents.at(i);
		if (charID == std::clamp(charID, 48, 57)) {
			numDigits.push_back(charID - 48);
		} else {
			if (charID == 45) {
				numIsNeg = true;
			} else {
				if (charID != 43) {
					break;
				}
			}
		}
	}

	int64_t num = 0;
	for (int i = 0; i < numDigits.size(); i++) {
		num += numDigits.at(i) * (int64_t)pow(10, (int64_t)numDigits.size() - i - 1);
	}
	if (numIsNeg)
		num = -num;
	if (num > INT32_MAX) {
		fprintf(stderr, "The Given Integer Value Is More Than The Limit %i\n", INT32_MAX);
		return false;
	}
	if (num < INT32_MIN) {
		fprintf(stderr, "The Given Integer Value Is Less Than The Limit %i\n", INT32_MIN);
		return false;
	}
	std::cout << (int)num << std::endl;
	//int n = 0;
	//sscanf_s(sceneContents.c_str(), "%i", &n);
	//std::cout << n << std::endl;

	return true;
}
