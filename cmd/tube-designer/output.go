package main

import (
	"fmt"
	"math"
	"sort"
	"strings"
	"time"
)

// prettyLen formats inches as feet and inches
func prettyLen(inches int) string {
	if inches < 0 {
		return fmt.Sprintf("-%s", prettyLen(-inches))
	}

	ft := inches / 12
	in := inches % 12

	if ft == 0 {
		return fmt.Sprintf("%d\"", in)
	}
	if in == 0 {
		return fmt.Sprintf("%d'", ft)
	}
	return fmt.Sprintf("%d'%d\"", ft, in)
}

// printResults prints all the different output formats
func printResults(tubing string, stockLen int, kerf float64, cuts []Cut, solution Solution) {
	fmt.Printf("\n=== Cut Optimization Results ===\n")
	fmt.Printf("Material: %s\n", tubing)
	fmt.Printf("Stock length: %s (%d inches)\n", prettyLen(stockLen), stockLen)
	fmt.Printf("Kerf: %.4f\"\n", kerf)
	fmt.Printf("Total cuts: %d\n", len(cuts))
	fmt.Printf("Sticks needed: %d\n\n", solution.NumSticks)

	// Summary statistics
	totalStock := solution.NumSticks * stockLen
	efficiency := float64(totalStock-solution.TotalWaste) / float64(totalStock) * 100

	fmt.Printf("=== Efficiency Summary ===\n")
	fmt.Printf("Total stock used: %s\n", prettyLen(totalStock))
	fmt.Printf("Total waste: %s\n", prettyLen(solution.TotalWaste))
	fmt.Printf("Material efficiency: %.1f%%\n", efficiency)
	fmt.Printf("Average waste per stick: %s\n\n", prettyLen(solution.TotalWaste/solution.NumSticks))

	// Detailed cut list
	fmt.Printf("=== Detailed Cut List ===\n")
	fmt.Printf("%-8s | %-50s | %-10s | %-10s\n", "Stick #", "Cuts (in order)", "Used", "Waste")
	fmt.Println(strings.Repeat("-", 85))

	for i, s := range solution.Sticks {
		var cutStrs []string
		for _, c := range s.Cuts {
			cutStrs = append(cutStrs, prettyLen(c.Length))
		}
		cutList := strings.Join(cutStrs, ", ")
		if len(cutList) > 48 {
			cutList = cutList[:45] + "..."
		}

		fmt.Printf("%-8d | %-50s | %-10s | %-10s\n",
			i+1,
			cutList,
			prettyLen(s.UsedLen),
			prettyLen(s.WasteLen))
	}

	// Cut ticket for shop floor
	fmt.Printf("\n\n=== Shop Floor Cut Ticket ===\n")
	fmt.Printf("Date: %s\n", time.Now().Format("2006-01-02"))
	fmt.Printf("Material: %s @ %s\n", tubing, prettyLen(stockLen))
	fmt.Printf("Kerf: %.4f\"\n", kerf)
	fmt.Printf("===============================================\n\n")

	kerfInt := int(math.Ceil(kerf * 1000))
	for i, s := range solution.Sticks {
		fmt.Printf("STICK #%d (%s stock):\n", i+1, prettyLen(stockLen))
		fmt.Println("─────────────────────────────")

		runningLen := 0
		for j, c := range s.Cuts {
			// Add kerf for the cut
			if j > 0 {
				runningLen += kerfInt / 1000
			}
			markAt := runningLen + c.Length

			fmt.Printf("  Cut %d: Mark at %s, Cut %s piece\n",
				j+1,
				prettyLen(markAt),
				prettyLen(c.Length))

			runningLen = markAt
		}
		fmt.Printf("  → Remaining: %s (waste)\n\n", prettyLen(s.WasteLen))
	}

	// Waste pieces summary
	if solution.NumSticks > 1 {
		fmt.Printf("\n=== Waste Pieces Summary ===\n")
		wasteMap := make(map[int]int)
		for _, s := range solution.Sticks {
			wasteMap[s.WasteLen]++
		}

		var wasteSizes []int
		for size := range wasteMap {
			wasteSizes = append(wasteSizes, size)
		}
		sort.Sort(sort.Reverse(sort.IntSlice(wasteSizes)))

		for _, size := range wasteSizes {
			count := wasteMap[size]
			fmt.Printf("  %s waste: %d piece%s\n",
				prettyLen(size),
				count,
				plural(count))
		}
	}
}

func plural(n int) string {
	if n == 1 {
		return ""
	}
	return "s"
}
