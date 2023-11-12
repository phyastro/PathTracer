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
	bool isNeg = false;
	int64_t number = 0;
	for (int i = 0; i < buffer.length(); i++) {
		int charID = (int)buffer.at(i);
		if (charID == std::clamp(charID, 48, 57)) {
			number = 10 * number + (charID - 48);
			try {
				bool isOutOfRange = isNeg ? (-number < INT32_MIN) : (number > INT32_MAX);
				if (isOutOfRange) {
					throw std::out_of_range("Exception: Integer Out Of Range");
				}
			}
			catch (std::out_of_range error) {
				std::cout << error.what() << std::endl;
				exit(EXIT_FAILURE);
			}
		} else {
			try {
				if ((i == 0) && ((charID == 45) || (charID == 43))) {
					if (charID == 45)
						isNeg = true;
				} else {
					throw std::invalid_argument("Exception: Invalid Integer");
				}
			}
			catch (std::invalid_argument error) {
				std::cout << error.what() << std::endl;
				exit(EXIT_FAILURE);
			}
		}
	}
	number = isNeg ? -number : number;

	return (int)number;
}

void loader::load() {
	std::string sceneDirectory = findCurrentDir();
	sceneDirectory.append("test.scene");
	std::string sceneContents = ReadFile(sceneDirectory);

	std::cout << parser::StrToInt(sceneContents) << std::endl;
}
