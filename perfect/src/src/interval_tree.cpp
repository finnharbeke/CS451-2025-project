#pragma once

#include <set>
#include <shared_mutex>

struct Interval {
    unsigned int left; // including
    unsigned int right; // including
    Interval(unsigned int l, unsigned int r) : left(l), right(r) {}

    bool operator<(const Interval& other) const noexcept {
        return right < other.left;
    }
    bool operator<(const unsigned int& a) const noexcept {
        return right < a;
    }
};

bool operator<(const unsigned int& a, const Interval& it) noexcept;
bool operator<(const unsigned int& a, const Interval& it) noexcept {
    return a < it.left;
}

class IntervalTree {
    public:
        IntervalTree() {}
        
        bool insert(unsigned int a) {
            std::lock_guard lock(m);
            auto l = S.find(a); // first check if
            if (l != S.end()) // already contained
                return false;
            l = S.find(a-1);
            auto r = S.find(a+1);
            if (l != S.end() && r != S.end()) { // [ll,lr]a[rl,rr]
                auto ll = l->left;
                auto rr = r->right;
                S.erase(l);
                S.erase(r);
                S.emplace(ll, rr);
            } else if (l != S.end()) {
                auto ll = l->left;
                S.erase(l);
                S.emplace(ll, a);
            } else if (r != S.end()) {
                auto rr = r->right;
                S.erase(r);
                S.emplace(a, rr);
            } else {
                S.emplace(a, a);
            }
            return true;
        }

        bool erase(unsigned int a, unsigned int b) {
            std::lock_guard lock(m);
            // we assume it's a subinterval
            auto l = S.find(a);
            auto r = S.find(b);
            if (l == S.end() || r == S.end() || l != r)
                return false;
            auto ll = l->left;
            auto rr = l->right;
            S.erase(l);
            if (ll < a) {
                S.emplace(ll, a - 1);
            }
            if (b < rr) {
                S.emplace(b + 1, rr);
            }
            return true;
        }

        struct ReadView {
            // keeps locked while in scope, and shared_lock allows multiple readers
            std::shared_lock<std::shared_mutex> lock;
            std::set<Interval, std::less<>>::const_iterator begin_it;
            std::set<Interval, std::less<>>::const_iterator end_it;

            ReadView(std::shared_mutex& m, const std::set<Interval, std::less<>>& s)
                : lock(m), begin_it(s.cbegin()), end_it(s.cend()) {}
            
            auto begin() const { return begin_it; }
            auto end()   const { return end_it; }
        };

        ReadView readView() const {
            return ReadView(m, S);
        }

        size_t size() {
            std::lock_guard lock(m);
            return S.size();
        }
    
    private:
        std::set<Interval, std::less<>> S;
        mutable std::shared_mutex m;
};