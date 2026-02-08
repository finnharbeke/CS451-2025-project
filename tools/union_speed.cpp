#include <iostream>
#include <set>
#include <algorithm>
#include <chrono>

void println(const std::set<int>& v)
{
    for (int i : v)
        std::cout << i << ' ';
    std::cout << '\n';
}

int main() {
    // Write C++ code here
    
    
    int n = 10000;
    
    std::set<unsigned int> v1, v2;
 
    std::chrono::steady_clock::duration ins(0), su(0);
    for (int j = 0; j < 20; j++) {
        for (int i = 0; i < n; i++) {
            //v1.insert(std::to_string(i));
            //v2.insert(std::to_string(2*i));
            v1.insert(i);
            v2.insert(2*i);
        }
        
        auto start = std::chrono::steady_clock::now();
        v1.insert(v2.begin(), v2.end());
        auto end = std::chrono::steady_clock::now();
        // println(v1.size());
        std::cout << v1.size() << std::endl;
        std::cout << ".insert took " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << "μs" << std::endl;
        ins += end - start;
    
        v1.clear();
        v2.clear();
        for (int i = 0; i < n; i++) {
            //v1.insert(std::to_string(i));
            //v2.insert(std::to_string(2*i));
            v1.insert(i);
            v2.insert(2*i);
        }
        
        start = std::chrono::steady_clock::now();

        std::set_union(v1.begin(), v1.end(),
        v2.begin(), v2.end(),
        inserter(v1, v1.begin()));
        end = std::chrono::steady_clock::now();
        std::cout << v1.size() << std::endl;
        std::cout << "set_union took " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << "μs" << std::endl;
        su += end - start;
    }

    std::cout << "insert took avg " << std::chrono::duration_cast<std::chrono::microseconds>(ins / 20).count() << "μs" << std::endl;
    std::cout << "set_union took avg " << std::chrono::duration_cast<std::chrono::microseconds>(su / 20).count() << "μs" << std::endl;


    return 0;
}