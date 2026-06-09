#include "dice.h"

#include <stdlib.h>
#include <time.h>

void cf_dice_seed(unsigned int seed) {
    srand(seed ? seed : (unsigned int)time(NULL));
}

void cf_roll_custom_dice(int *die_a, int *die_b, int *sum) {
    static const int a[6] = {1, 2, 3, 4, 1, 2};
    static const int b[6] = {1, 2, 3, 1, 2, 3};
    int da = a[rand() % 6];
    int db = b[rand() % 6];
    if (die_a) *die_a = da;
    if (die_b) *die_b = db;
    if (sum) *sum = da + db;
}
