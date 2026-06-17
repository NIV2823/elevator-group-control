#include "utils.h"

static std::random_device rd;
static std::mt19937 gen(rd());

double randomDouble(double minVal, double maxVal)
{
    std::uniform_real_distribution<double> dist(minVal, maxVal);
    return dist(gen);
}

double randomDouble()
{
    return randomDouble(0.0, 1.0);
}

int randomInt(int minVal, int maxVal)
{
    std::uniform_int_distribution<int> dist(minVal, maxVal);
    return dist(gen);
}

int poissonRandom(double lambda)
{
    if (lambda <= 0.0) return 0;
    if (lambda > 30.0)
    {
        // 大lambda用正态近似
        std::normal_distribution<double> dist(lambda, std::sqrt(lambda));
        int val = (int)std::round(dist(gen));
        return val < 0 ? 0 : val;
    }
    std::poisson_distribution<int> dist(lambda);
    return dist(gen);
}

double clamp(double val, double lo, double hi)
{
    if (val < lo) return lo;
    if (val > hi) return hi;
    return val;
}

int clampInt(int val, int lo, double hi)
{
    if (val < lo) return lo;
    if (val > (int)hi) return (int)hi;
    return val;
}

void reseedRandom(unsigned int seed)
{
    gen.seed(seed);
}
