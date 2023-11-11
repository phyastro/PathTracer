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

int loader::parser::StrToInt(std::string buffer) {
	std::vector<int> numDigits;
	bool numIsNeg = false;
	for (int i = 0; i < buffer.length(); i++) {
		int charID = (int)buffer.at(i);
		if (charID == std::clamp(charID, 48, 57)) {
			numDigits.push_back(charID - 48);
		} else {
			if (charID == 45) {
				numIsNeg = true;
			} else {
				try {
					if (charID != 43) {
						throw std::invalid_argument("Exception: Invalid Integer");
					}
				}
				catch (std::invalid_argument error) {
					std::cout << error.what() << std::endl;
					exit(EXIT_FAILURE);
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
	try {
		if ((num > INT32_MAX) || (num < INT32_MIN)) {
			throw std::out_of_range("Exception: Integer Out Of Range");
		}
	}
	catch (std::out_of_range error) {
		std::cout << error.what() << std::endl;
		exit(EXIT_FAILURE);
	}
	
	return (int)num;
}

void loader::load() {
	std::string sceneDirectory = findCurrentDir();
	sceneDirectory.append("test.scene");
	std::string sceneContents = ReadFile(sceneDirectory);

	std::cout << parser::StrToInt(sceneContents) << std::endl;
}
