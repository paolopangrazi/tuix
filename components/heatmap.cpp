#include "heatmap.hpp"

void heat_rgb(double t, int& R, int& G, int& B) {
    if (t < 0) t = 0; else if (t > 1) t = 1;
    struct Stop { double p; int r, g, b; };
    static const Stop s[] = {
        {0.00,  40,  70, 200},   // cold  — blue
        {0.25,  40, 180, 210},   // cyan
        {0.50,  70, 190,  90},   // green
        {0.75, 235, 205,  60},   // yellow
        {1.00, 220,  55,  45},   // hot   — red
    };
    for (int i = 1; i < 5; ++i) {
        if (t <= s[i].p) {
            const double f = (t - s[i - 1].p) / (s[i].p - s[i - 1].p);
            R = int(s[i - 1].r + f * (s[i].r - s[i - 1].r) + 0.5);
            G = int(s[i - 1].g + f * (s[i].g - s[i - 1].g) + 0.5);
            B = int(s[i - 1].b + f * (s[i].b - s[i - 1].b) + 0.5);
            return;
        }
    }
    R = 220; G = 55; B = 45;
}
