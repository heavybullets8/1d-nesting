package main

import "testing"

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
