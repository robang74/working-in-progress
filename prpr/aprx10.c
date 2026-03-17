#include <stdio.h>
#include <stdint.h>

static inline uint32_t last_digit(uint32_t x) {
    return x - ((x * 0xcccccccdULL) >> 35) * 10;
}

int main() {

    for(uint8_t k = 1; k < 32; k++) {
        uint32_t count[10] = {0};

        uint32_t seed = 123456789;
        for (uint64_t i = 0; i < 10000000; i++) {
            // un PRNG qualsiasi (qui un LCG semplice per esempio)
            seed = seed * 1664525 + 1013904223;
            uint32_t d = last_digit(seed ^ (seed >> k));
            count[d]++;
        }

        double min = 1e9, max = -1e9;
        for (int i = 0; i < 10; i++) {
            double df = ((count[i] / 1e5) - 10) * 1e5;
            printf("%d → %10u  (%+6.1f ppk)\n", i, count[i], df);
            if(min > df) min = df;
            else
            if(max < df) max = df;
        }

        printf("dist %2d: %5.1lf (%lf %lf)\n", k, max - min, min, max);
    
    }
    return 0;
}
