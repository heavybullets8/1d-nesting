package main

import (
	"fmt"
	"html/template"
	"math"
	"os"
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

// Pattern groups sticks with identical cuts
type Pattern struct {
	Cuts     []Cut
	Count    int
	UsedLen  int
	WasteLen int
}

// groupPatterns combines sticks that have the same sequence of cuts
func groupPatterns(sticks []Stick) []Pattern {
	m := make(map[string]*Pattern)
	for _, s := range sticks {
		var parts []string
		for _, c := range s.Cuts {
			parts = append(parts, fmt.Sprintf("%d", c.Length))
		}
		key := strings.Join(parts, "-")

		if p, ok := m[key]; ok {
			p.Count++
		} else {
			m[key] = &Pattern{
				Cuts:     s.Cuts,
				Count:    1,
				UsedLen:  s.UsedLen,
				WasteLen: s.WasteLen,
			}
		}
	}

	patterns := make([]Pattern, 0, len(m))
	for _, p := range m {
		patterns = append(patterns, *p)
	}

	sort.Slice(patterns, func(i, j int) bool {
		if patterns[i].Count == patterns[j].Count {
			return patterns[i].UsedLen < patterns[j].UsedLen
		}
		return patterns[i].Count > patterns[j].Count
	})

	return patterns
}

// generateHTML writes a printable HTML file summarizing the solution
func generateHTML(filename, tubing string, stockLen int, kerf float64, cuts []Cut, solution Solution) error {
	type cutInstr struct {
		Mark string
		Len  string
	}

	type patternData struct {
		Count   int
		CutList string
		Used    string
		Waste   string
		Instr   []cutInstr
	}

	type pageData struct {
		Date       string
		Tubing     string
		Stock      string
		Kerf       string
		TotalStock string
		TotalWaste string
		Efficiency string
		AvgWaste   string
		Patterns   []patternData
	}

	kerfInt := int(math.Ceil(kerf * 1000))
	patterns := groupPatterns(solution.Sticks)

	var patData []patternData
	for _, p := range patterns {
		var cutStrs []string
		for _, c := range p.Cuts {
			cutStrs = append(cutStrs, prettyLen(c.Length))
		}
		cutList := strings.Join(cutStrs, ", ")

		runningLen := 0
		var instr []cutInstr
		for i, c := range p.Cuts {
			if i > 0 {
				runningLen += kerfInt / 1000
			}
			markAt := runningLen + c.Length
			instr = append(instr, cutInstr{
				Mark: prettyLen(markAt),
				Len:  prettyLen(c.Length),
			})
			runningLen = markAt
		}

		patData = append(patData, patternData{
			Count:   p.Count,
			CutList: cutList,
			Used:    prettyLen(p.UsedLen),
			Waste:   prettyLen(p.WasteLen),
			Instr:   instr,
		})
	}

	totalStock := solution.NumSticks * stockLen
	efficiency := float64(totalStock-solution.TotalWaste) / float64(totalStock) * 100

	data := pageData{
		Date:       time.Now().Format("2006-01-02"),
		Tubing:     tubing,
		Stock:      prettyLen(stockLen),
		Kerf:       fmt.Sprintf("%.4f\"", kerf),
		TotalStock: prettyLen(totalStock),
		TotalWaste: prettyLen(solution.TotalWaste),
		Efficiency: fmt.Sprintf("%.1f", efficiency),
		AvgWaste:   prettyLen(solution.TotalWaste / solution.NumSticks),
		Patterns:   patData,
	}

	const tpl = `<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="utf-8">
    <title>Cut Plan</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; }
        h1, h2, h3 { color: #333; }
        table { border-collapse: collapse; width: 100%; margin-bottom: 20px; }
        th, td { border: 1px solid #ccc; padding: 8px; text-align: left; }
        th { background-color: #f2f2f2; }
    </style>
</head>
<body>
<h1>Cut Plan</h1>
<p>Date: {{.Date}}</p>
<p>Material: {{.Tubing}} @ {{.Stock}}</p>
<p>Kerf: {{.Kerf}}</p>

<h2>Efficiency Summary</h2>
<ul>
    <li>Total stock used: {{.TotalStock}}</li>
    <li>Total waste: {{.TotalWaste}}</li>
    <li>Material efficiency: {{.Efficiency}}%</li>
    <li>Average waste per stick: {{.AvgWaste}}</li>
</ul>

<h2>Cut Patterns</h2>
<table>
    <tr><th>Qty</th><th>Cuts</th><th>Used</th><th>Waste</th></tr>
    {{range .Patterns}}
    <tr>
        <td>{{.Count}}</td>
        <td>{{.CutList}}</td>
        <td>{{.Used}}</td>
        <td>{{.Waste}}</td>
    </tr>
    {{end}}
</table>

{{range $idx, $p := .Patterns}}
<h3>Pattern {{$idx | inc}} – Qty {{$p.Count}}</h3>
<table>
    <tr><th>#</th><th>Mark At</th><th>Cut Piece</th></tr>
    {{range $i, $c := $p.Instr}}
    <tr><td>{{$i | inc}}</td><td>{{$c.Mark}}</td><td>{{$c.Len}}</td></tr>
    {{end}}
    <tr><td colspan="3">Remaining: {{$p.Waste}}</td></tr>
</table>
{{end}}

</body>
</html>`

	t := template.Must(template.New("page").Funcs(template.FuncMap{"inc": func(i int) int { return i + 1 }}).Parse(tpl))

	f, err := os.Create(filename)
	if err != nil {
		return err
	}
	defer f.Close()

	return t.Execute(f, data)
}
