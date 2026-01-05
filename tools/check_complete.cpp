#include <vector>
#include <fstream>
#include <string>
#include <iostream>
#include <format>

void check_complete_sufficient(int p, int m, int n) {
    std::cout << "checking process " << p << ", n = " << n << ", m = "<< m << std::endl;
    
    std::vector<std::vector<bool> > seen;
    for (int i = 0; i <= n; i++) {
        seen.push_back(std::vector<bool>(m+1, false));
    }
    
    char buf[256];
    snprintf(buf, sizeof(buf), "../example/output/proc%02d.output", p);
    std::cout << "reading " << buf << "... ";
    std::ifstream file(buf);
    int x;
    int sender;
    std::string bd;
    int lines = 0;
    while (file >> bd) {
        if (!bd.compare("b")) {
            file >> x;
            if (x >= 1 && x <= m)
                seen[0][x] = true;
            else {
                std::cout << bd << " " << x << std::endl;
            }
        } else {
            file >> sender;
            file >> x;
            // std::cout << bd << " " << sender << " " << x << std::endl;
            if (x >= 1 && x <= m) {
                if (seen[sender][x]) {
                    std::cout << "already seen: " << bd << " " << sender << " " << x << std::endl;
                }
                seen[sender][x] = true;
            } else {
                std::cout << bd << " " << sender << " " << x << std::endl;
            }
        }
        lines ++;
        // if (x > 30)
        //     std::exit(0);
    }
    std::cout << lines << " lines read!" << std::endl;
    // std::exit(0);

    std::vector<int > missing;
    for (int i = 0; i <= n; i++) {
        missing.push_back(0);
    }

    for (int s = 0; s <= n; ++s) {

        for (int i = 1; i <= m; ++i) {
            if (!seen[s][i]) {
                missing[s] += 1;
                // if (s == 0)
                //     std::cout << "Missing in broadcast: " << i << "\n";
                // else
                //     std::cout << "Missing from " << s << ": " << i << "\n";
            }
        }
    }

    for (int s = 0; s <= n; ++s) {
        if (missing[s] == 0) continue;
        if (s == 0)
            std::cout << p << ">? not sent:  " << missing[s] << std::endl;
        else
            std::cout << s << ">" << p << " not deliv: " << missing[s] << std::endl;
    }
}

void check_order(int p, int m, int n) {
    std::cout << "checking process " << p << ", n = " << n << ", m = "<< m << std::endl;
    
    std::vector<int> last;
    for (int i = 0; i <= n; i++) {
        last.push_back(0);
    }
    
    char buf[256];
    snprintf(buf, sizeof(buf), "../example/output/proc%02d.output", p);
    std::cout << "reading " << buf << "... ";
    std::ifstream file(buf);
    int x;
    int sender;
    std::string bd;
    int lines = 0;
    while (file >> bd) {
        if (!bd.compare("b")) {
            file >> x;
            if (x == last[0] + 1)
                last[0] = x;
            else {
                std::cout << bd << " " << x << ", expected " << last[0] + 1 << std::endl;
            }
        } else {
            file >> sender;
            file >> x;
            // std::cout << bd << " " << sender << " " << x << std::endl;
            if (x == last[sender] + 1) {
                last[sender] = x;
            } else {
                std::cout << bd << " " << x << ", expected " << last[sender] + 1 << std::endl;
            }
        }
        lines ++;
        // if (x > 30)
        //     std::exit(0);
    }
    std::cout << lines << " lines read!" << std::endl;
    // std::exit(0);
    std::cout << "last "<< p << " -> ? " << last.at(0) << std::endl;
    for (int i = 1; i <= n; i++) {
        std::cout << "last "<< i << " -> " << p << " " << last.at(i) << std::endl;
    }
}

int main(int argc, char* argv[]) {
    int n = strtol(argv[1], nullptr, 10);
    int m = strtol(argv[2], nullptr, 10);

    for (auto p = 1; p <= n; p++) {
        // check_complete_sufficient(p, m, n);
        check_order(p, m, n);

    }

    return 0;
}