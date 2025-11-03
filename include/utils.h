#ifndef UTILS_H
#define UTILS_H

// Helper to find the greatest common divisor for fraction simplification
constexpr int gcd(int a, int b) { return b == 0 ? a : gcd(b, a % b); }

#endif // UTILS_H
