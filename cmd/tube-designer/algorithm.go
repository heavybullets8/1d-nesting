package main

import (
	"fmt"
	"math"

	"github.com/nextmv-io/sdk/mip"
)

// optimizeCutting uses the Nextmv MIP API with the HiGHS back-end to find
// the optimal cutting plan.
func optimizeCutting(cuts []Cut, stockLen int, kerf float64) Solution {
	// --- 1. Generate all possible cutting patterns ---
	allCuts := make([]int, len(cuts))
	for i, c := range cuts {
		allCuts[i] = c.Length
	}
	patterns := generatePatterns(allCuts, stockLen, kerf)
	if len(patterns) == 0 {
		fmt.Println("Error: no valid cutting patterns could be generated.")
		return Solution{}
	}

	// --- 2. Build the MIP model ---
	model := mip.NewModel()
	model.Objective().SetMinimize() // minimise number of sticks

	patternVars := make([]mip.Var, len(patterns))
	for i := range patterns {
		patternVars[i] = model.NewInt(0, math.MaxInt64)
		model.Objective().NewTerm(1.0, patternVars[i]) // cost = 1 per stick
	}

	// Track demand for each cut length
	cutDemand := make(map[int]int)
	for _, cut := range cuts {
		cutDemand[cut.Length]++
	}

	// Add ≥-demand constraints for every cut length
	for cutLen, demand := range cutDemand {
		constr := model.NewConstraint(mip.GreaterThanOrEqual, float64(demand))
		for i, p := range patterns {
			count := 0
			for _, piece := range p {
				if piece == cutLen {
					count++
				}
			}
			if count > 0 {
				constr.NewTerm(float64(count), patternVars[i])
			}
		}
	}

	// --- 3. Solve with HiGHS ---
	solver, err := mip.NewSolver(mip.Highs, model)
	if err != nil {
		fmt.Printf("HiGHS solver init error: %v\n", err)
		return Solution{}
	}

	solution, err := solver.Solve(mip.SolveOptions{}) // ← only this line changed
	if err != nil {
		fmt.Printf("Solve error: %v\n", err)
		return Solution{}
	}
	if !solution.IsOptimal() {
		fmt.Println("Warning: solver did not prove optimality.")
	}

	// --- 4. Convert solver output to our domain structs ---
	result := Solution{}
	totalUsed := 0

	for i, p := range patterns {
		numSticks := int(math.Round(solution.Value(patternVars[i])))
		if numSticks == 0 {
			continue
		}

		cutSlice := make([]Cut, len(p))
		usedLen := 0
		for j, cl := range p {
			cutSlice[j] = Cut{Length: cl}
			usedLen += cl
		}
		if len(cutSlice) > 1 {
			usedLen += int(math.Round(float64(len(cutSlice)-1) * kerf))
		}

		for s := 0; s < numSticks; s++ {
			result.Sticks = append(result.Sticks, Stick{
				Cuts:     cutSlice,
				StockLen: stockLen,
				UsedLen:  usedLen,
				WasteLen: stockLen - usedLen,
			})
		}
		totalUsed += usedLen * numSticks
	}

	result.NumSticks = len(result.Sticks)
	result.TotalWaste = result.NumSticks*stockLen - totalUsed
	return result
}

// generatePatterns finds all possible ways a single stick can be cut.
func generatePatterns(availableCuts []int, stockLen int, kerf float64) [][]int {
	uniqueCutsMap := make(map[int]bool)
	for _, c := range availableCuts {
		uniqueCutsMap[c] = true
	}
	uniqueCuts := []int{}
	for c := range uniqueCutsMap {
		uniqueCuts = append(uniqueCuts, c)
	}

	var patterns [][]int
	var currentPattern []int
	var find func(int, int)
	kerfInt := int(math.Round(kerf))

	find = func(startIndex, remainingLen int) {
		if len(currentPattern) > 0 {
			pCopy := make([]int, len(currentPattern))
			copy(pCopy, currentPattern)
			patterns = append(patterns, pCopy)
		}

		for i := startIndex; i < len(uniqueCuts); i++ {
			cut := uniqueCuts[i]
			kerfCost := 0
			if len(currentPattern) > 0 {
				kerfCost = kerfInt
			}
			if remainingLen >= cut+kerfCost {
				currentPattern = append(currentPattern, cut)
				find(i, remainingLen-(cut+kerfCost))
				currentPattern = currentPattern[:len(currentPattern)-1]
			}
		}
	}

	find(0, stockLen)
	return patterns
}
