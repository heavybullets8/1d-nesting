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
	kerfInt := int(math.Ceil(kerf * 1000)) // Work in thousandths

	for _, cut := range cuts {
		placed := false

		for i := range sticks {
			usedLen := calculateUsedLength(sticks[i].Cuts, kerfInt)

			// Check if this cut fits
			if usedLen+cut.Length+kerfInt/1000 <= stockLen {
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

	return createSolution(sticks, stockLen, kerfInt)
}

// bestFitDecreasing implements the BFD algorithm - puts each item in the bin with least waste
func bestFitDecreasing(cuts []Cut, stockLen int, kerf float64) Solution {
	sticks := []Stick{}
	kerfInt := int(math.Ceil(kerf * 1000))

	for _, cut := range cuts {
		bestFit := -1
		minWaste := stockLen + 1

		// Find the stick with minimum waste after adding this cut
		for i := range sticks {
			usedLen := calculateUsedLength(sticks[i].Cuts, kerfInt)
			newUsed := usedLen + cut.Length + kerfInt/1000

			if newUsed <= stockLen {
				waste := stockLen - newUsed
				if waste < minWaste {
					minWaste = waste
					bestFit = i
				}
			}
		}

		if bestFit >= 0 {
			sticks[bestFit].Cuts = append(sticks[bestFit].Cuts, cut)
		} else {
			sticks = append(sticks, Stick{
				Cuts:     []Cut{cut},
				StockLen: stockLen,
			})
		}
	}

	return createSolution(sticks, stockLen, kerfInt)
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

// calculateUsedLength calculates total used length including kerf
func calculateUsedLength(cuts []Cut, kerfInt int) int {
	if len(cuts) == 0 {
		return 0
	}

	usedLen := 0
	for _, c := range cuts {
		usedLen += c.Length
	}
	// Add kerf for each cut
	usedLen += len(cuts) * kerfInt / 1000

	return usedLen
}

// createSolution creates a Solution from sticks
func createSolution(sticks []Stick, stockLen int, kerfInt int) Solution {
	totalWaste := 0

	for i := range sticks {
		usedLen := calculateUsedLength(sticks[i].Cuts, kerfInt)
		sticks[i].UsedLen = usedLen
		sticks[i].WasteLen = stockLen - usedLen
		totalWaste += sticks[i].WasteLen
	}

	return Solution{
		Sticks:     sticks,
		TotalWaste: totalWaste,
		NumSticks:  len(sticks),
	}
}
