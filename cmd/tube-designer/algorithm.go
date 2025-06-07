package main

import (
	"fmt"
	"math"
	"os"
	"os/signal"
	"sort"
	"syscall"
	"time"
)

// Global flag to handle graceful shutdown
var searchCancelled = false

// optimizeCutting always uses branch-and-bound after getting initial BFD solution
func optimizeCutting(cuts []Cut, stockLen int, kerf float64) Solution {
	// Set up signal handling for Ctrl+C
	setupSignalHandler()

	// First, get a good initial solution using BFD
	initialSolution := bestFitDecreasing(cuts, stockLen, kerf)

	// Calculate theoretical lower bound
	totalLength := 0
	for _, cut := range cuts {
		totalLength += cut.Length
	}
	kerfTh := int(math.Ceil(kerf * 1000))
	totalLength *= 1000                     // convert to thousandths
	totalLength += (len(cuts) - 1) * kerfTh // minimum kerf if all cuts were in one long tube
	theoreticalMin := int(math.Ceil(float64(totalLength) / float64(stockLen*1000)))

	fmt.Printf("\nInitial solution: %d sticks\n", initialSolution.NumSticks)
	fmt.Printf("Theoretical minimum: %d sticks\n", theoreticalMin)
	fmt.Printf("Starting optimization... (Press Ctrl+C to stop and use current best solution)\n")
	fmt.Println("=== Search Progress ===")

	// If initial solution is already at theoretical minimum, we're done
	if initialSolution.NumSticks <= theoreticalMin {
		fmt.Println("Already at theoretical minimum!")
		return initialSolution
	}

	// Run branch-and-bound with the initial solution as upper bound
	optimized := branchAndBoundWithProgress(cuts, stockLen, kerf, initialSolution, theoreticalMin)

	// Apply simple improvement step
	finalSolution := iterativeImprovement(optimized, stockLen, kerf)

	fmt.Printf("\nOptimization complete!\n")
	fmt.Printf("Final solution: %d sticks (saved %d)\n",
		finalSolution.NumSticks,
		initialSolution.NumSticks-finalSolution.NumSticks)

	return finalSolution
}

// setupSignalHandler configures graceful shutdown on Ctrl+C
func setupSignalHandler() {
	c := make(chan os.Signal, 1)
	signal.Notify(c, os.Interrupt, syscall.SIGTERM)

	go func() {
		<-c
		fmt.Printf("\n\nReceived Ctrl+C - stopping search and using current best solution...\n")
		searchCancelled = true
	}()
}

// branchAndBoundWithProgress runs branch-and-bound with live progress display
func branchAndBoundWithProgress(cuts []Cut, stockLen int, kerf float64, initialSolution Solution, theoreticalMin int) Solution {
	kerfTh := int(math.Ceil(kerf * 1000))
	stockTh := stockLen * 1000

	// Convert and sort cuts
	sortedCuts := make([]indexedCut, len(cuts))
	for i, c := range cuts {
		sortedCuts[i] = indexedCut{
			index:  i,
			length: c.Length * 1000,
			cut:    c,
		}
	}
	sort.Slice(sortedCuts, func(i, j int) bool {
		return sortedCuts[i].length > sortedCuts[j].length
	})

	// Calculate total material needed for efficiency calculation
	totalMaterialNeeded := 0
	for _, cut := range cuts {
		totalMaterialNeeded += cut.Length
	}

	// Setup branch-and-bound state
	state := &bbState{
		cuts:                sortedCuts,
		stockTh:             stockTh,
		kerfTh:              kerfTh,
		bestBins:            initialSolution.NumSticks,
		bestPacking:         nil,
		theoreticalMin:      theoreticalMin,
		startTime:           time.Now(),
		nodeCount:           0,
		totalMaterialNeeded: totalMaterialNeeded,
		stockLen:            stockLen,
		lastProgressUpdate:  time.Now(),
		progressUpdateFreq:  time.Second,
	}

	// Convert initial solution to packing format for starting point
	initialPacking := convertSolutionToPacking(initialSolution, sortedCuts)
	if initialPacking != nil {
		state.bestPacking = initialPacking
	}

	// Display initial progress
	state.displayProgress()

	// Run branch-and-bound
	currentPacking := make([][]indexedCut, 0)
	branchAndBoundContinuous(state, 0, currentPacking)

	fmt.Printf("\nExplored %d nodes in %.2f seconds\n", state.nodeCount, time.Since(state.startTime).Seconds())

	// If no better solution found, use initial
	if state.bestPacking == nil {
		return initialSolution
	}

	// Convert optimal packing to Solution
	sticks := make([]Stick, len(state.bestPacking))
	for i, bin := range state.bestPacking {
		sticks[i] = Stick{
			Cuts:     make([]Cut, len(bin)),
			StockLen: stockLen,
		}
		for j, ic := range bin {
			sticks[i].Cuts[j] = ic.cut
		}
	}

	return createSolution(sticks, stockLen, kerfTh)
}

// convertSolutionToPacking converts a Solution back to packing format
func convertSolutionToPacking(solution Solution, sortedCuts []indexedCut) [][]indexedCut {
	// This is a helper to preserve good solutions
	// For simplicity, we'll return nil and let branch-and-bound find its own solution
	return nil
}

// indexedCut tracks cuts during optimization
type indexedCut struct {
	index  int
	length int // in thousandths
	cut    Cut
}

// bbState maintains state for branch-and-bound search
type bbState struct {
	cuts                []indexedCut
	stockTh             int
	kerfTh              int
	bestBins            int
	bestPacking         [][]indexedCut
	theoreticalMin      int
	startTime           time.Time
	nodeCount           int
	totalMaterialNeeded int
	stockLen            int
	lastProgressUpdate  time.Time
	progressUpdateFreq  time.Duration
}

// displayProgress shows current search status
func (state *bbState) displayProgress() {
	elapsed := time.Since(state.startTime)

	// Calculate current efficiency
	totalStock := state.bestBins * state.stockLen
	efficiency := float64(state.totalMaterialNeeded) / float64(totalStock) * 100

	fmt.Printf("\rSticks: %d | Efficiency: %.1f%% | Nodes: %d | Time: %s",
		state.bestBins,
		efficiency,
		state.nodeCount,
		formatDuration(elapsed))
}

// formatDuration formats a duration in a human-readable way
func formatDuration(d time.Duration) string {
	if d < time.Minute {
		return fmt.Sprintf("%.1fs", d.Seconds())
	} else if d < time.Hour {
		return fmt.Sprintf("%.1fm", d.Minutes())
	} else {
		hours := int(d.Hours())
		minutes := int(d.Minutes()) % 60
		return fmt.Sprintf("%dh%dm", hours, minutes)
	}
}

// branchAndBoundContinuous explores packings until manually cancelled
func branchAndBoundContinuous(state *bbState, cutIndex int, currentPacking [][]indexedCut) {
	// Check for cancellation
	if searchCancelled {
		return
	}

	state.nodeCount++

	// Update progress display every second
	if time.Since(state.lastProgressUpdate) >= state.progressUpdateFreq {
		state.displayProgress()
		state.lastProgressUpdate = time.Now()
	}

	// Base case: all cuts placed
	if cutIndex >= len(state.cuts) {
		if len(currentPacking) < state.bestBins {
			state.bestBins = len(currentPacking)
			state.bestPacking = deepCopyPacking(currentPacking)

			// Update progress immediately when we find a better solution
			state.displayProgress()

			// If we reached theoretical minimum, we can't do better
			if state.bestBins <= state.theoreticalMin {
				fmt.Printf("\nReached theoretical minimum! Stopping search.\n")
				searchCancelled = true
				return
			}
		}
		return
	}

	// Pruning: can't improve if already using too many bins
	if len(currentPacking) >= state.bestBins {
		return
	}

	// Enhanced pruning: if we're far from placing all cuts and already near limit
	remainingCuts := len(state.cuts) - cutIndex
	if remainingCuts > 10 && len(currentPacking) >= state.bestBins-1 {
		return
	}

	// Calculate tighter lower bound
	remainingLength := 0
	maxRemaining := 0
	for i := cutIndex; i < len(state.cuts); i++ {
		remainingLength += state.cuts[i].length
		if state.cuts[i].length > maxRemaining {
			maxRemaining = state.cuts[i].length
		}
	}

	// Consider minimum kerf needed
	minKerfNeeded := 0
	if remainingCuts > 1 {
		// At minimum, largest remaining cut needs its own bin
		minKerfNeeded = (remainingCuts - 1) * state.kerfTh
	}

	lowerBound := int(math.Ceil(float64(remainingLength+minKerfNeeded) / float64(state.stockTh)))

	// Prune if lower bound exceeds best solution
	if len(currentPacking)+lowerBound >= state.bestBins {
		return
	}

	currentCut := state.cuts[cutIndex]

	// Try placing in existing bins (try best fit order)
	binFits := make([]struct {
		idx   int
		waste int
	}, 0)

	for binIdx := range currentPacking {
		if canFitInBin(state, currentPacking[binIdx], currentCut) {
			usedLength := 0
			for _, c := range currentPacking[binIdx] {
				usedLength += c.length
			}
			usedLength += len(currentPacking[binIdx]) * state.kerfTh
			usedLength += currentCut.length
			waste := state.stockTh - usedLength

			binFits = append(binFits, struct {
				idx   int
				waste int
			}{binIdx, waste})
		}
	}

	// Sort by waste (best fit first)
	sort.Slice(binFits, func(i, j int) bool {
		return binFits[i].waste < binFits[j].waste
	})

	// Try bins in best-fit order
	for _, bf := range binFits {
		if searchCancelled {
			return
		}
		currentPacking[bf.idx] = append(currentPacking[bf.idx], currentCut)
		branchAndBoundContinuous(state, cutIndex+1, currentPacking)
		currentPacking[bf.idx] = currentPacking[bf.idx][:len(currentPacking[bf.idx])-1]

		// Early termination if we found theoretical minimum
		if state.bestBins <= state.theoreticalMin || searchCancelled {
			return
		}
	}

	// Try creating a new bin
	if len(currentPacking)+1 < state.bestBins && !searchCancelled {
		newBin := []indexedCut{currentCut}
		newPacking := append(currentPacking, newBin)
		branchAndBoundContinuous(state, cutIndex+1, newPacking)
	}
}

// canFitInBin checks if a cut fits in the given bin with kerf
func canFitInBin(state *bbState, bin []indexedCut, cut indexedCut) bool {
	if len(bin) == 0 {
		return cut.length <= state.stockTh
	}

	usedLength := 0
	for _, c := range bin {
		usedLength += c.length
	}
	// Add kerf between existing cuts
	usedLength += (len(bin) - 1) * state.kerfTh

	// Check if adding new cut with kerf fits
	newLength := usedLength + state.kerfTh + cut.length
	return newLength <= state.stockTh
}

// deepCopyPacking creates a deep copy of the bin packing
func deepCopyPacking(packing [][]indexedCut) [][]indexedCut {
	result := make([][]indexedCut, len(packing))
	for i, bin := range packing {
		result[i] = make([]indexedCut, len(bin))
		copy(result[i], bin)
	}
	return result
}

// bestFitDecreasing uses BFD heuristic for initial solution
func bestFitDecreasing(cuts []Cut, stockLen int, kerf float64) Solution {
	// Sort cuts by length descending
	sortedCuts := make([]Cut, len(cuts))
	copy(sortedCuts, cuts)
	sort.Slice(sortedCuts, func(i, j int) bool {
		return sortedCuts[i].Length > sortedCuts[j].Length
	})

	sticks := []Stick{}
	kerfTh := int(math.Ceil(kerf * 1000))
	stockTh := stockLen * 1000

	// Place each cut in the bin with least remaining space that fits
	for _, cut := range sortedCuts {
		bestIdx := -1
		minWaste := stockTh + 1

		for i := range sticks {
			usedTh := calculateUsedLength(sticks[i].Cuts, kerfTh)
			newUsed := usedTh + cut.Length*1000
			if len(sticks[i].Cuts) > 0 {
				newUsed += kerfTh
			}

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

// iterativeImprovement tries to reduce bin count by moving cuts
func iterativeImprovement(solution Solution, stockLen int, kerf float64) Solution {
	kerfTh := int(math.Ceil(kerf * 1000))
	stockTh := stockLen * 1000

	improved := true
	maxIterations := 3
	iteration := 0

	for improved && iteration < maxIterations {
		improved = false
		iteration++

		// Try to empty bins by redistributing their cuts
		for i := 0; i < len(solution.Sticks); i++ {
			if len(solution.Sticks[i].Cuts) == 0 {
				continue
			}

			// Can we redistribute all cuts from this bin?
			canRedistribute := true
			placements := make([]int, len(solution.Sticks[i].Cuts))

			for j, cut := range solution.Sticks[i].Cuts {
				placed := false
				for k := 0; k < len(solution.Sticks); k++ {
					if k == i {
						continue
					}

					usedTh := calculateUsedLength(solution.Sticks[k].Cuts, kerfTh)
					newUsed := usedTh + cut.Length*1000
					if len(solution.Sticks[k].Cuts) > 0 {
						newUsed += kerfTh
					}

					if newUsed <= stockTh {
						placements[j] = k
						placed = true
						break
					}
				}

				if !placed {
					canRedistribute = false
					break
				}
			}

			// If we can redistribute all cuts, do it
			if canRedistribute {
				// Move cuts to their new locations
				for j := len(solution.Sticks[i].Cuts) - 1; j >= 0; j-- {
					cut := solution.Sticks[i].Cuts[j]
					targetBin := placements[j]
					solution.Sticks[targetBin].Cuts = append(solution.Sticks[targetBin].Cuts, cut)
				}

				// Remove the now-empty bin
				solution.Sticks = append(solution.Sticks[:i], solution.Sticks[i+1:]...)
				solution.NumSticks--
				improved = true
				break
			}
		}
	}

	// Recalculate solution metrics
	return createSolution(solution.Sticks, stockLen, kerfTh)
}

// calculateUsedLength computes total length including kerf
func calculateUsedLength(cuts []Cut, kerfTh int) int {
	if len(cuts) == 0 {
		return 0
	}

	usedTh := 0
	for _, c := range cuts {
		usedTh += c.Length * 1000
	}

	// Kerf only between cuts (n-1 gaps)
	gaps := len(cuts) - 1
	if gaps > 0 {
		usedTh += gaps * kerfTh
	}
	return usedTh
}

// createSolution builds a Solution from sticks
func createSolution(sticks []Stick, stockLen int, kerfTh int) Solution {
	totalWaste := 0

	for i := range sticks {
		usedTh := calculateUsedLength(sticks[i].Cuts, kerfTh)
		sticks[i].UsedLen = usedTh / 1000
		sticks[i].WasteLen = stockLen - sticks[i].UsedLen
		totalWaste += sticks[i].WasteLen
	}

	return Solution{
		Sticks:     sticks,
		TotalWaste: totalWaste,
		NumSticks:  len(sticks),
	}
}
