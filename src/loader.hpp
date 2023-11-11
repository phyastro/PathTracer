#include <iostream>
#include <sstream>
#include <fstream>
#include <filesystem>

namespace loader
{
    std::string findCurrentDir();
    const std::string ReadFile(std::string);
    namespace parser
    {
        int StrToInt(std::string);
    };
    void load();
};
