#include "genetic_drift.h"

#include "random_gen.h"

std::pair<double, double> calculate_drift_probabilities(const unsigned long runs, const unsigned N, const unsigned K, const double p)
{
    if (runs == 0)
    {
        if (p == 0.0)
        {
            return {1.0, 0.0};
        }
        if (p == 1.0)
        {
            return {0.0, 1.0};
        }
        return {0.0, 0.0};
    }
    unsigned long count_disappearance_blue = 0;
    unsigned long count_disappearance_white = 0;
    const unsigned long long count_alleles = static_cast<unsigned long long>(N) * 2;
    for (unsigned long run = 0; run < runs; ++run)
    {
        double probability_blue = p;
        for (unsigned generation = 0; generation < K; ++generation)
        {
            unsigned long long count_blue = 0;
            for (unsigned long long allele = 0; allele < count_alleles; ++allele)
            {
                if (get_random_number() <= probability_blue)
                {
                    ++count_blue;
                }
            }
            if (count_blue == 0)
            {
                ++count_disappearance_blue;
                break;
            }
            // fixation blue => disappearance white
            if (count_blue == count_alleles)
            {
                ++count_disappearance_white;
                break;
            }
            probability_blue = static_cast<double>(count_blue) / count_alleles;
        }
    }
    return {static_cast<double>(count_disappearance_blue) / runs, static_cast<double>(count_disappearance_white) / runs};
}
