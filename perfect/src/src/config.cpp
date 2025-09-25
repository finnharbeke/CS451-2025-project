#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

struct PerfectConfig {
    public:
        unsigned long m, i;
        PerfectConfig(const char* configPath) {
            std::ifstream configFile(configPath);
            if (!configFile.is_open()) {
                std::ostringstream os;
                os << "`" << configPath << "` does not exist.";
                throw std::invalid_argument(os.str());
            }
            
            if (!(configFile >> m >> i)) {
                std::ostringstream os;
                os << "Parsing for `" << configPath << "` failed.";
                throw std::invalid_argument(os.str());
            }
        }
};