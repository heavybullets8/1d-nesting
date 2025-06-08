package main

import (
	"bufio"
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
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
	fmt.Printf("--- Tube-Designer %s ---\n", Version)

	// 1. Tubing description
	tubing := getInput(r, "Tubing type (e.g. 2x2)", "2x2")

	// 2. Stock length
	stockStr := getInput(r, "Stock length (e.g. 24' or 288)", "24'")
	stockIn := parseAdvancedLength(stockStr)
	if stockIn <= 0 {
		fmt.Fprintln(os.Stderr, "Error: Stock length must be a positive number.")
		os.Exit(1)
	}
	fmt.Printf("  ✓ Using %s stock\n", prettyLen(stockIn))

	// 3. Kerf size
	kerfStr := getInput(r, "Kerf/blade thickness (e.g. 1/8 or 0.125)", "1/8")
	kerfIn := parseFraction(kerfStr)
	if kerfIn <= 0 {
		kerfIn = 0.125 // Default to 1/8"
		fmt.Printf("  ✓ Using default kerf: %.3f\"\n", kerfIn)
	} else {
		fmt.Printf("  ✓ Using %.4f\" kerf\n", kerfIn)
	}

	// 4. Cut list
	fmt.Println("\nEnter cuts as 'length quantity' (e.g., '90 25' or '7'6 50').")
	fmt.Println("Press Enter on a blank line to finish.")

	cuts := []Cut{}
	cutID := 1

	for {
		fmt.Print("→ ")
		line, _ := r.ReadString('\n')
		line = strings.TrimSpace(line)
		if line == "" {
			break
		}

		parts := strings.Fields(line)
		if len(parts) < 2 {
			fmt.Println("  ✖ Invalid format. Please use 'length quantity'.")
			continue
		}

		qtyStr := parts[len(parts)-1]
		qty, err := strconv.Atoi(qtyStr)
		if err != nil || qty <= 0 {
			fmt.Println("  ✖ Quantity must be a positive number.")
			continue
		}

		lengthStr := strings.Join(parts[:len(parts)-1], " ")
		length := parseAdvancedLength(lengthStr)
		if length <= 0 {
			fmt.Println("  ✖ Could not parse length.")
			continue
		}

		if length > stockIn {
			fmt.Printf("  ✖ Cut of %s is longer than stock of %s.\n",
				prettyLen(length), prettyLen(stockIn))
			continue
		}

		for i := 0; i < qty; i++ {
			cuts = append(cuts, Cut{Length: length, ID: cutID})
			cutID++
		}
		fmt.Printf("  ✓ Added %d × %s\n", qty, prettyLen(length))
	}

	if len(cuts) == 0 {
		fmt.Println("No cuts entered. Exiting.")
		return
	}

	// 5. Optimize
	fmt.Printf("\nOptimizing %d total cuts...\n", len(cuts))

	startTime := time.Now()
	solution := optimizeCutting(cuts, stockIn, kerfIn)
	elapsed := time.Since(startTime)

	fmt.Printf("Optimization finished in %.2f seconds.\n", elapsed.Seconds())

	// 6. Print results
	printResults(tubing, stockIn, kerfIn, cuts, solution)

	htmlFile := "cut_plan.html"
	if err := generateHTML(htmlFile, tubing, stockIn, kerfIn, cuts, solution); err != nil {
		fmt.Fprintf(os.Stderr, "\nError writing HTML file: %v\n", err)
	} else {
		fmt.Printf("\nDetailed cut plan saved to %s\n", htmlFile)
		if err := openFile(htmlFile); err != nil {
			fmt.Fprintf(os.Stderr, "Could not open HTML file automatically: %v\n", err)
		}
	}
}

// getInput prompts the user and returns their input, or a default value.
func getInput(r *bufio.Reader, prompt, defaultValue string) string {
	fmt.Printf("%s: ", prompt)
	input, _ := r.ReadString('\n')
	input = strings.TrimSpace(input)
	if input == "" {
		return defaultValue
	}
	return input
}

func openFile(name string) error {
	absPath, err := filepath.Abs(name)
	if err != nil {
		absPath = name
	}

	var cmd *exec.Cmd
	switch runtime.GOOS {
	case "windows":
		cmd = exec.Command("rundll32", "url.dll,FileProtocolHandler", absPath)
	case "darwin":
		cmd = exec.Command("open", absPath)
	default: // Linux / *nix
		if exec.Command("which", "xdg-open").Run() == nil {
			cmd = exec.Command("xdg-open", absPath)
		} else {
			return fmt.Errorf("could not find a browser opener; open %s manually", absPath)
		}
	}
	return cmd.Start()
}
