#pragma once

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include <set>
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

struct FIFOConfig {
    public:
        unsigned long m;
        FIFOConfig(const char* configPath) {
            std::ifstream configFile(configPath);
            if (!configFile.is_open()) {
                std::ostringstream os;
                os << "`" << configPath << "` does not exist.";
                throw std::invalid_argument(os.str());
            }
            
            if (!(configFile >> m)) {
                std::ostringstream os;
                os << "Parsing for `" << configPath << "` failed.";
                throw std::invalid_argument(os.str());
            }
        }
};

class LatticeConfig {
    public:
        unsigned int p;
        unsigned char vs;
        unsigned short ds;
        std::vector<std::vector<int>> proposals; 

    LatticeConfig(const char* configPath) {
        configFile = std::ifstream(configPath);
        if (!configFile.is_open()) {
            std::ostringstream os;
            os << "`" << configPath << "` does not exist.";
            throw std::invalid_argument(os.str());
        }
        
        if (!(configFile >> p >> vs >> ds)) {
            std::ostringstream os;
            os << "Parsing for `" << configPath << "` failed.";
            throw std::invalid_argument(os.str());
        }

        std::string tmp;
        std::getline(configFile, tmp);

        closed = false;
    }

    std::set<unsigned int>* next_proposal() {
        if (closed) {
            return new std::set<unsigned int>();
        }
        std::string line;
        std::getline(configFile, line);
        std::istringstream iss(line);
        auto vec = new std::set<unsigned int>();
        unsigned int x;
        while (iss >> x) {
            vec->emplace(x);
        }
        return vec;
    }

    private:
        std::ifstream configFile;
        bool closed = true;
};