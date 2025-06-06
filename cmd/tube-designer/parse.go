package main

import (
	"math"
	"regexp"
	"strconv"
	"strings"
)

// parseAdvancedLength parses various length formats
func parseAdvancedLength(s string) int {
	s = strings.TrimSpace(s)
	if s == "" {
		return 0
	}

	// Regex to match: optional feet, optional inches, optional fraction
	// Examples: 19' 6 1/2", 180 1/2, 24', 288
	re := regexp.MustCompile(`(?i)^\s*(?:(\d+)')?(?:\s*(\d+))?(?:\s+(\d+)/(\d+))?(?:\s*")?(?:\s|$)`)

	// Check if it's just a plain number (inches)
	if num, err := strconv.Atoi(s); err == nil {
		return num
	}

	// Check for feet notation
	if strings.Contains(s, "'") {
		matches := re.FindStringSubmatch(s)
		if matches != nil {
			inches := 0

			// Feet
			if matches[1] != "" {
				feet, _ := strconv.Atoi(matches[1])
				inches += feet * 12
			}

			// Inches
			if matches[2] != "" {
				in, _ := strconv.Atoi(matches[2])
				inches += in
			}

			// Fraction
			if matches[3] != "" && matches[4] != "" {
				num, _ := strconv.Atoi(matches[3])
				den, _ := strconv.Atoi(matches[4])
				if den > 0 {
					inches += int(math.Round(float64(num) / float64(den)))
				}
			}

			return inches
		}
	}

	// Try parsing as "inches fraction" format (e.g., "180 1/2")
	parts := strings.Fields(s)
	if len(parts) == 2 {
		if inches, err := strconv.Atoi(parts[0]); err == nil {
			frac := parseFraction(parts[1])
			if frac > 0 {
				return inches + int(math.Round(frac))
			}
		}
	}

	return 0
}

// parseFraction parses a fraction or decimal
func parseFraction(s string) float64 {
	s = strings.TrimSpace(s)

	// Check for fraction
	if strings.Contains(s, "/") {
		parts := strings.Split(s, "/")
		if len(parts) == 2 {
			num, err1 := strconv.ParseFloat(strings.TrimSpace(parts[0]), 64)
			den, err2 := strconv.ParseFloat(strings.TrimSpace(parts[1]), 64)
			if err1 == nil && err2 == nil && den != 0 {
				return num / den
			}
		}
	}

	// Try as decimal
	if val, err := strconv.ParseFloat(s, 64); err == nil {
		return val
	}

	return 0
}
