#include <vector>
#include <fstream>
#include <string>
#include <iostream>

int main() {
    int m = 500000;
    int n = 5;
    std::vector<std::vector<bool> > seen;
    for (int i = 0; i <= n; i++) {
        seen.push_back(std::vector<bool>(m+1, false));
    }

    std::ifstream file("../example/output/proc01.output");
    int x;
    int sender;
    std::string b;
    int lines = 0;
    while (file >> b) {
        file >> sender;
        file >> x;
        // std::cout << b << " " << sender << " " << x << std::endl;
        if (x >= 1 && x <= m)
            seen[sender][x] = true;
        else {
            std::cout << b << " " << sender << " " << x << std::endl;
        }
        lines ++;
        // if (x > 30)
        //     std::exit(0);
    }
    std::cout << lines << std::endl;
    // std::exit(0);

    for (int s = 2; s <= n; ++s) {
        for (int i = 1; i <= m; ++i) {
            if (!seen[s][i]) {
                std::cout << "Missing in " << s << ": " << i << "\n";
            }
        }
    }

    return 0;
}