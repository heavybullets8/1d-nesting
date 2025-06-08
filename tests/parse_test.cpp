#include "parse.h"
#include <cassert>
#include <cmath>

int main() {
    // parseAdvancedLength tests
    assert(parseAdvancedLength("24'") == 288);
    assert(parseAdvancedLength("288") == 288);
    assert(parseAdvancedLength("20' 6\"") == 246);
    assert(parseAdvancedLength("7'6 1/2\"") == 91);
    assert(parseAdvancedLength("180 1/2") == 181);
    assert(parseAdvancedLength("8'4\"") == 100);
    assert(parseAdvancedLength("bad") == 0);

    // parseFraction tests
    assert(std::abs(parseFraction("1/2") - 0.5) < 1e-6);
    assert(std::abs(parseFraction("0.125") - 0.125) < 1e-6);
    assert(std::abs(parseFraction("3/16") - 0.1875) < 1e-6);
    assert(std::abs(parseFraction(" 3 / 6 ") - 0.5) < 1e-6);
    assert(parseFraction("junk") == 0.0);

    return 0;
}
