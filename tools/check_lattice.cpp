#include <vector>
#include <fstream>
#include <string>
#include <iostream>
#include <format>
#include <set>
#include <string>
#include <sstream>
#include <memory>

bool is_subset(std::set<unsigned int>& sub, std::set<unsigned int>& supe) {
    for (auto x : sub) {
        if (supe.find(x) == supe.end()) {
            return false;
        }
    }
    return true;
}

std::ostream& operator<<(std::ostream& os, std::set<unsigned int>& s);
std::ostream& operator<<(std::ostream& os, std::set<unsigned int>& s) {
    for (auto x : s)
        os << x << " ";
    return os;
}

void check_shot(unsigned int p_id, std::vector<std::set<unsigned int> >& Ii,
        std::vector<std::set<unsigned int> >& Oi,
        std::set<unsigned int>& Iijoin) {

    bool good = true;
    // validity
    unsigned short i = 1;
    auto itI = Ii.begin(), itO = Oi.begin();
    for (; itI != Ii.end() && itO != Oi.end(); (++itI, ++itO, ++i)) {

        if (itO->size() == 0) // crashed
            continue;
        
        if (!is_subset(*itI, *itO)) {
            if (good)
                std::cout << "shot " << p_id << ":" << std::endl;
            std::cout << "Ii ⊈ Oi for i = " << i
            << ", Ii = " << (*itI) << "and Oi = " << (*itO) << std::endl;
            good = false;
        }
        if (!is_subset(*itO, Iijoin)) {
            if (good)
                std::cout << "shot " << p_id << ":" << std::endl;
            std::cout << "Oi ⊈ ⋃Ii for i = " << i
            << ", Oi = " << (*itO) << "and ⋃Ii = " << Iijoin << std::endl;
            good = false;
        }
    }

    // consistency
    i = 1;
    unsigned short j = 1;
    auto it1 = Oi.begin(), it2 = Oi.begin();
    for (; it1 != Oi.end(); (++it1, ++i)) {
        for (; it2 != Oi.end(); (++it2, ++j)) {
            if (it1->size() == 0 || it2->size() == 0) // crashed
                continue;
            if (!is_subset(*it1, *it2) && !is_subset(*it2, *it1)) {
                if (good)
                    std::cout << "shot " << p_id << ":" << std::endl;
                std::cout << "Oi ⊈ Oj and Oj ⊈ Oi for i, j = " << i << ", " << j
                << ", Oi = " << (*it1) << "and Oj = " << (*it2) << std::endl;
                good = false;
            }
        }
    }
    // if (good) {
    //     std::cout << " ... all good!" << std::endl;
    // }
}

int main(int argc, char* argv[]) {
    unsigned int p = strtol(argv[1], nullptr, 10);
    unsigned int n = strtol(argv[2], nullptr, 10);

    std::vector<std::unique_ptr<std::ifstream> > outputs;
    std::vector<std::unique_ptr<std::ifstream> > configs;
    for (auto i = 1; i <= p; i++) {
        char buf[256];
        snprintf(buf, sizeof(buf), "../example/output/proc%02d.output", i);
        outputs.emplace_back(std::make_unique<std::ifstream>(buf));
        char buf2[256];
        snprintf(buf2, sizeof(buf2), "../example/output/proc%02d.config", i);
        configs.emplace_back(std::make_unique<std::ifstream>(buf2));

        // remove top line
        std::string tmp;
        std::getline(*configs.back(), tmp);
    }

    std::set<unsigned short> crashed;

    for (unsigned int p_id = 1; p_id <= n; p_id ++) {

        std::vector<std::set<unsigned int> > Ii;
        std::vector<std::set<unsigned int> > Oi;
        std::set<unsigned int> Iijoin;

        unsigned short process = 1;
        auto ito = outputs.begin(), itc = configs.begin();
        for (; ito != outputs.end() && itc != configs.end(); (++ito, ++itc, ++process)) {
    
            std::string cline;
            std::getline(**itc, cline);
            std::istringstream cstream(cline);
            Ii.emplace_back();
            unsigned int x;
            while (cstream >> x) {
                Ii.back().insert(x);
            }
            std::string oline;
            std::getline(**ito, oline);
            std::istringstream ostream(oline);
            Oi.emplace_back();
            if (oline.length() == 0) {
                if (crashed.find(process) == crashed.end()) {
                    std::cout << "process " << process << " crashed before shot "
                    << p_id << std::endl;
                    crashed.insert(process);
                }
            } else {
                while (ostream >> x) {
                    Oi.back().insert(x);
                }
            }
            Iijoin.insert(Ii.back().begin(), Ii.back().end());
        }

        // check_complete_sufficient(p, m, n);
        check_shot(p_id, Ii, Oi, Iijoin);
    }

    return 0;
}