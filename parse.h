#ifndef PARSE_H
#define PARSE_H

#include <string>

// Parse various length formats (e.g., "24'", "8'4\"", "180 1/2", "288")
int parseAdvancedLength(const std::string &s);

// Parse a fraction or decimal (e.g., "1/2", "0.125", "3/16")
double parseFraction(const std::string &s);

// Format inches as feet and inches (e.g., 100 -> "8'4\"")
std::string prettyLen(int inches);

// Get user input with a default value
std::string getInput(const std::string &prompt,
                     const std::string &defaultValue = "");

#ifdef __EMSCRIPTEN__
// Functions used by the web UI to preload inputs and retrieve the output HTML
extern "C" {
void wasmSetInput(const char *data);
const char *wasmGetOutput();
}
#endif


#endif // PARSE_H
