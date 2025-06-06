package main

import (
	"bufio"
	"fmt"
	"math"
	"os"
	"regexp"
	"sort"
	"strconv"
	"strings"
	"time"
)

// Program version information—populated at build time through the Makefile.
var (
	Version   = "dev"
	Commit    = "none"
	BuildTime = "unknown"
)

// Cut represents a single cut piece
type Cut struct {
	Length int
	ID     int
}

// Stick represents a stock piece with its cuts
type Stick struct {
	Cuts     []Cut
	StockLen int
	UsedLen  int
	WasteLen int
}

// Solution represents a cutting solution
type Solution struct {
	Sticks     []Stick
	TotalWaste int
	NumSticks  int
}

func main() {
	r := bufio.NewReader(os.Stdin)
	fmt.Printf("Tube-Designer %s (%s) built %s\n\n", Version, Commit, BuildTime)

	// 1. Tubing description
	fmt.Print("Tubing type (e.g. 2x2, 2x3): ")
	tubing, _ := r.ReadString('\n')
	tubing = strings.TrimSpace(tubing)

	// 2. Stock length
	fmt.Println("\nStock length examples:")
	fmt.Println("  24'        (24 feet)")
	fmt.Println("  288        (288 inches)")
	fmt.Println("  20' 6\"     (20 feet 6 inches)")
	fmt.Print("\nLength of stock: ")
	stockStr, _ := r.ReadString('\n')
	stockIn := parseAdvancedLength(strings.TrimSpace(stockStr))
	if stockIn <= 0 {
		fmt.Fprintln(os.Stderr, "Stock length must be positive")
		os.Exit(1)
	}
	fmt.Printf("  ✓ Stock length: %s (%d inches)\n", prettyLen(stockIn), stockIn)

	// 3. Kerf size
	fmt.Println("\nKerf examples:")
	fmt.Println("  1/8        (one eighth inch)")
	fmt.Println("  0.125      (decimal inches)")
	fmt.Println("  3/16       (three sixteenths)")
	fmt.Print("\nKerf / blade thickness: ")
	kerfStr, _ := r.ReadString('\n')
	kerfIn := parseFraction(strings.TrimSpace(kerfStr))
	if kerfIn <= 0 {
		kerfIn = 0.125 // Default to 1/8"
		fmt.Printf("  ⚠ Using default kerf: %.3f\"\n", kerfIn)
	} else {
		fmt.Printf("  ✓ Kerf: %.4f\"\n", kerfIn)
	}

	// 4. Cut list
	fmt.Println("\nEnter cuts (length quantity). Examples:")
	fmt.Println("  90 25           (25 pieces at 90 inches)")
	fmt.Println("  7'6 50          (50 pieces at 7 feet 6 inches)")
	fmt.Println("  7' 6 1/2\" 20   (20 pieces at 7 feet 6.5 inches)")
	fmt.Println("  12' 10          (10 pieces at 12 feet)")
	fmt.Println("Press Enter on blank line to finish.")

	cuts := []Cut{}
	cutID := 1

	for {
		fmt.Print("→ ")
		line, _ := r.ReadString('\n')
		line = strings.TrimSpace(line)
		if line == "" {
			break
		}

		// Find the last number (quantity)
		parts := strings.Fields(line)
		if len(parts) < 2 {
			fmt.Println("  ✖ Need both length and quantity")
			continue
		}

		// Last part should be quantity
		qtyStr := parts[len(parts)-1]
		qty, err := strconv.Atoi(qtyStr)
		if err != nil || qty <= 0 {
			fmt.Println("  ✖ Last value should be quantity (positive number)")
			continue
		}

		// Everything else is the length
		lengthStr := strings.Join(parts[:len(parts)-1], " ")
		length := parseAdvancedLength(lengthStr)
		if length <= 0 {
			fmt.Println("  ✖ Couldn't parse length")
			continue
		}

		// Check if cut is longer than stock
		if length > stockIn {
			fmt.Printf("  ✖ Cut length %s is longer than stock %s\n",
				prettyLen(length), prettyLen(stockIn))
			continue
		}

		// Add cuts
		for i := 0; i < qty; i++ {
			cuts = append(cuts, Cut{Length: length, ID: cutID})
			cutID++
		}

		fmt.Printf("  ✓ Added %d pieces of %s\n", qty, prettyLen(length))
	}

	if len(cuts) == 0 {
		fmt.Println("No cuts entered – nothing to do.")
		return
	}

	// 5. Optimize
	fmt.Printf("\nOptimizing %d cuts...\n", len(cuts))

	startTime := time.Now()
	solution := optimizeCutting(cuts, stockIn, kerfIn)
	elapsed := time.Since(startTime)

	fmt.Printf("Optimization complete in %.2f seconds\n", elapsed.Seconds())

	// 6. Print results
	printResults(tubing, stockIn, kerfIn, cuts, solution)
}

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
