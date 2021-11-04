#include "genetic_drift.h"

#include <iostream>
#include <string>

int main(int argc, char ** argv)
{
    unsigned long runs = 10000;
    unsigned N = 100;
    unsigned K = 1000;
    double p = 0.3;
    if (argc > 1) {
        runs = std::stoul(argv[1]);
        if (argc > 2) {
            N = std::stoul(argv[2]);
            if (argc > 3) {
                K = std::stoul(argv[3]);
                if (argc > 4) {
                    p = std::stod(argv[4]);
                }
            }
        }
    }
    const auto [d, f] = calculate_drift_probabilities(runs, N, K, p);
    std::cout << "disappearance probability: " << d
        << "\nfixation probability: " << f << "\n";
}
