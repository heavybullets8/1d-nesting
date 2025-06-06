package main

import (
	"math"
	"sort"
)

// optimizeCutting uses a branch-and-bound approach for better optimization
func optimizeCutting(cuts []Cut, stockLen int, kerf float64) Solution {
	// Sort cuts by length (descending)
	sort.Slice(cuts, func(i, j int) bool { return cuts[i].Length > cuts[j].Length })

	// First, get a baseline solution using FFD
	baselineSolution := firstFitDecreasing(cuts, stockLen, kerf)

	// If we have few enough cuts, try more exhaustive search
	if len(cuts) <= 15 {
		// Try different orderings to find better solutions
		bestSolution := baselineSolution

		// Try a few heuristics
		heuristics := []func([]Cut){
			// Already sorted by length descending
			func(c []Cut) {},
			// Sort by length ascending
			func(c []Cut) { sort.Slice(c, func(i, j int) bool { return c[i].Length < c[j].Length }) },
			// Alternate large and small
			func(c []Cut) { alternateLargeSmall(c) },
		}

		for _, h := range heuristics {
			cutsCopy := make([]Cut, len(cuts))
			copy(cutsCopy, cuts)
			h(cutsCopy)

			solution := firstFitDecreasing(cutsCopy, stockLen, kerf)
			if solution.TotalWaste < bestSolution.TotalWaste {
				bestSolution = solution
			}
		}

		return bestSolution
	}

	// For larger problems, use enhanced FFD with best-fit
	return bestFitDecreasing(cuts, stockLen, kerf)
}

// firstFitDecreasing implements the FFD algorithm
func firstFitDecreasing(cuts []Cut, stockLen int, kerf float64) Solution {
	sticks := []Stick{}
	kerfTh := int(math.Ceil(kerf * 1000)) // kerf in thousandths
	stockTh := stockLen * 1000            // stock length in thousandths

	for _, cut := range cuts {
		placed := false

		for i := range sticks {
			usedTh := calculateUsedLength(sticks[i].Cuts, kerfTh)
			newUsed := usedTh + cut.Length*1000 + kerfTh // add kerf AFTER the cut
			if newUsed <= stockTh {
				sticks[i].Cuts = append(sticks[i].Cuts, cut)
				placed = true
				break
			}
		}

		if !placed {
			sticks = append(sticks, Stick{
				Cuts:     []Cut{cut},
				StockLen: stockLen,
			})
		}
	}

	return createSolution(sticks, stockLen, kerfTh)
}

// least remaining space (again using thousandths for accuracy).
func bestFitDecreasing(cuts []Cut, stockLen int, kerf float64) Solution {
	sticks := []Stick{}
	kerfTh := int(math.Ceil(kerf * 1000))
	stockTh := stockLen * 1000

	for _, cut := range cuts {
		bestIdx := -1
		minWaste := stockTh + 1

		for i := range sticks {
			usedTh := calculateUsedLength(sticks[i].Cuts, kerfTh)
			newUsed := usedTh + cut.Length*1000 + kerfTh
			if newUsed <= stockTh {
				waste := stockTh - newUsed
				if waste < minWaste {
					minWaste = waste
					bestIdx = i
				}
			}
		}

		if bestIdx >= 0 {
			sticks[bestIdx].Cuts = append(sticks[bestIdx].Cuts, cut)
		} else {
			sticks = append(sticks, Stick{
				Cuts:     []Cut{cut},
				StockLen: stockLen,
			})
		}
	}
	return createSolution(sticks, stockLen, kerfTh)
}

// alternateLargeSmall reorders cuts to alternate between large and small
func alternateLargeSmall(cuts []Cut) {
	n := len(cuts)
	temp := make([]Cut, n)
	copy(temp, cuts)

	sort.Slice(temp, func(i, j int) bool { return temp[i].Length > temp[j].Length })

	left, right := 0, n-1
	for i := 0; i < n; i++ {
		if i%2 == 0 {
			cuts[i] = temp[left]
			left++
		} else {
			cuts[i] = temp[right]
			right--
		}
	}
}

// Kerf is counted only *between* cuts (gaps = n-1).
func calculateUsedLength(cuts []Cut, kerfTh int) int {
	if len(cuts) == 0 {
		return 0
	}

	usedTh := 0
	for _, c := range cuts {
		usedTh += c.Length * 1000 // convert inches → thousandths
	}

	gaps := len(cuts) - 1
	if gaps > 0 {
		usedTh += gaps * kerfTh
	}
	return usedTh
}

// createSolution creates a Solution from sticks
// back to whole inches for reporting.
func createSolution(sticks []Stick, stockLen int, kerfTh int) Solution {
	totalWaste := 0

	for i := range sticks {
		usedTh := calculateUsedLength(sticks[i].Cuts, kerfTh)
		sticks[i].UsedLen = usedTh / 1000 // back to inches
		sticks[i].WasteLen = stockLen - sticks[i].UsedLen
		totalWaste += sticks[i].WasteLen
	}

	return Solution{
		Sticks:     sticks,
		TotalWaste: totalWaste,
		NumSticks:  len(sticks),
	}
}
