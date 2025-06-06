package main

import (
	"bufio"
	"fmt"
	"os"
	"os/exec"
	"runtime"
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

	// Also generate an HTML version for the operator
	htmlFile := "cut_plan.html"
	if err := generateHTML(htmlFile, tubing, stockIn, kerfIn, cuts, solution); err != nil {
		fmt.Fprintf(os.Stderr, "Failed to write HTML output: %v\n", err)
	} else {
		fmt.Printf("HTML output written to %s\n", htmlFile)
		if err := openFile(htmlFile); err != nil {
			fmt.Fprintf(os.Stderr, "Failed to open HTML file: %v\n", err)
		}
	}
}

func openFile(name string) error {
	var cmd *exec.Cmd
	switch runtime.GOOS {
	case "windows":
		cmd = exec.Command("cmd", "/c", "start", "", name)
	case "darwin":
		cmd = exec.Command("open", name)
	default:
		cmd = exec.Command("xdg-open", name)
	}
	return cmd.Start()
}
