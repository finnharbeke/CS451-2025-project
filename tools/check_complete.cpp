#include <vector>
#include <fstream>
#include <string>
#include <iostream>
#include <format>

void check(int p, int m, int n) {
    std::cout << "checking process " << p << ", n = " << n << ", m = "<< m << std::endl;
    
    std::vector<std::vector<bool> > seen;
    for (int i = 0; i <= n; i++) {
        seen.push_back(std::vector<bool>(m+1, false));
    }
    
    char buf[256];
    snprintf(buf, sizeof(buf), "../example/output/proc%02d.output", p);
    std::cout << "path " << buf << std::endl;
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
    std::cout << lines << std::endl;
    // std::exit(0);

    for (int s = 0; s <= n; ++s) {
        for (int i = 1; i <= m; ++i) {
            if (!seen[s][i]) {
                if (s == 0)
                    std::cout << "Missing in broadcast: " << i << "\n";
                else
                    std::cout << "Missing from " << s << ": " << i << "\n";
            }
        }
    }
}

int main(int argc, char* argv[]) {
    int n = strtol(argv[1], nullptr, 10);
    int m = strtol(argv[2], nullptr, 10);

    for (auto p = 1; p <= n; p++)
        check(p, m, n);

    return 0;
}