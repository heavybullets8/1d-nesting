package main

import (
	"testing"
)

func TestPrettyLen(t *testing.T) {
	cases := []struct {
		in  int
		out string
	}{
		{0, "0\""},
		{12, "1'"},
		{14, "1'2\""},
		{25, "2'1\""},
		{-5, "-5\""},
	}
	for _, c := range cases {
		if got := prettyLen(c.in); got != c.out {
			t.Errorf("prettyLen(%d)=%q want %q", c.in, got, c.out)
		}
	}
}

func TestParseFraction(t *testing.T) {
	cases := []struct {
		in  string
		out float64
	}{
		{"1/2", 0.5},
		{"0.125", 0.125},
		{"3/16", 0.1875},
		{" 3 / 6 ", 0.5},
		{"junk", 0},
	}
	for _, c := range cases {
		if got := parseFraction(c.in); got != c.out {
			t.Errorf("parseFraction(%q)=%.4f want %.4f", c.in, got, c.out)
		}
	}
}

func TestParseAdvancedLength(t *testing.T) {
	cases := []struct {
		in  string
		out int
	}{
		{"24'", 288},
		{"288", 288},
		{"20' 6\"", 246},
		{"7'6 1/2\"", 91},
		{"180 1/2", 181},
		{"8'4\"", 100}, // Added for the user's case
		{"bad", 0},
	}
	for _, c := range cases {
		if got := parseAdvancedLength(c.in); got != c.out {
			t.Errorf("parseAdvancedLength(%q)=%d want %d", c.in, got, c.out)
		}
	}
}

func TestGroupPatterns(t *testing.T) {
	sticks := []Stick{
		{Cuts: []Cut{{Length: 5}, {Length: 5}}, UsedLen: 10, WasteLen: 2},
		{Cuts: []Cut{{Length: 5}, {Length: 5}}, UsedLen: 10, WasteLen: 2},
		{Cuts: []Cut{{Length: 10}}, UsedLen: 10, WasteLen: 1},
	}
	patterns := groupPatterns(sticks)
	if len(patterns) != 2 {
		t.Fatalf("expected 2 patterns got %d", len(patterns))
	}
	if patterns[0].Count != 2 {
		t.Errorf("first pattern count=%d want 2", patterns[0].Count)
	}
	if patterns[1].Count != 1 {
		t.Errorf("second pattern count=%d want 1", patterns[1].Count)
	}
}

func TestOptimizeCutting270Cuts(t *testing.T) {
	// Test the user's exact scenario: 270 cuts for 2x2 tubing
	cuts := []Cut{}
	cutID := 1

	// 80 pieces of 5' (60 inches)
	for i := 0; i < 80; i++ {
		cuts = append(cuts, Cut{Length: 60, ID: cutID})
		cutID++
	}

	// 100 pieces of 8'4" (100 inches)
	for i := 0; i < 100; i++ {
		cuts = append(cuts, Cut{Length: 100, ID: cutID})
		cutID++
	}

	// 90 pieces of 3' (36 inches)
	for i := 0; i < 90; i++ {
		cuts = append(cuts, Cut{Length: 36, ID: cutID})
		cutID++
	}

	stockLen := 288 // 24' in inches
	kerf := 0.0625  // 1/16"

	solution := optimizeCutting(cuts, stockLen, kerf)

	// Verify basic constraints
	if solution.NumSticks <= 0 {
		t.Errorf("NumSticks should be positive, got %d", solution.NumSticks)
	}

	// Verify all cuts are included
	totalCuts := 0
	for _, stick := range solution.Sticks {
		totalCuts += len(stick.Cuts)
	}
	if totalCuts != 270 {
		t.Errorf("Expected 270 cuts in solution, got %d", totalCuts)
	}

	// Verify no stick is overfilled
	kerfTh := int(kerf * 1000)
	for i, stick := range solution.Sticks {
		usedTh := calculateUsedLength(stick.Cuts, kerfTh)
		if usedTh > stockLen*1000 {
			t.Errorf("Stick %d overfilled: used %d thousandths, max %d", i, usedTh, stockLen*1000)
		}
	}

	// Calculate efficiency
	totalStock := solution.NumSticks * stockLen
	efficiency := float64(totalStock-solution.TotalWaste) / float64(totalStock) * 100

	t.Logf("270-cut optimization results:")
	t.Logf("  Sticks needed: %d", solution.NumSticks)
	t.Logf("  Total waste: %d inches", solution.TotalWaste)
	t.Logf("  Efficiency: %.1f%%", efficiency)

	// The solution should be reasonably efficient (>85%)
	if efficiency < 85 {
		t.Errorf("Efficiency too low: %.1f%% (expected >85%%)", efficiency)
	}
}

func TestOptimizeCuttingComparisonSmall(t *testing.T) {
	// Small test to verify optimization improves on simple greedy
	cuts := []Cut{
		{Length: 120, ID: 1},
		{Length: 60, ID: 2},
		{Length: 60, ID: 3},
		{Length: 50, ID: 4},
		{Length: 50, ID: 5},
		{Length: 40, ID: 6},
	}

	stockLen := 240
	kerf := 0.125

	// Get optimized solution
	solution := optimizeCutting(cuts, stockLen, kerf)

	// This specific case should fit in 2 sticks optimally:
	// Stick 1: 120 + 60 + 60 (with kerf)
	// Stick 2: 50 + 50 + 40 (with kerf)
	if solution.NumSticks > 2 {
		t.Errorf("Expected 2 sticks for optimal solution, got %d", solution.NumSticks)
	}
}

func TestLargeCutListPerformance(t *testing.T) {
	// Test with a large number of cuts to ensure it completes quickly
	cuts := []Cut{}
	cutID := 1

	// Create 500 cuts of various sizes
	sizes := []int{120, 100, 80, 60, 48, 36, 24}
	for i := 0; i < 500; i++ {
		size := sizes[i%len(sizes)]
		cuts = append(cuts, Cut{Length: size, ID: cutID})
		cutID++
	}

	stockLen := 288
	kerf := 0.125

	// This should complete quickly even with 500 cuts
	solution := optimizeCutting(cuts, stockLen, kerf)

	if solution.NumSticks <= 0 {
		t.Errorf("Invalid solution: NumSticks = %d", solution.NumSticks)
	}

	// Verify all cuts are included
	totalCuts := 0
	for _, stick := range solution.Sticks {
		totalCuts += len(stick.Cuts)
	}
	if totalCuts != 500 {
		t.Errorf("Expected 500 cuts in solution, got %d", totalCuts)
	}
}
