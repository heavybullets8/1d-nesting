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
	if inches == 0 {
		return `0"`
	}
	ft := inches / 12
	in := inches % 12
	if ft == 0 {
		return fmt.Sprintf(`%d"`, in)
	}
	if in == 0 {
		return fmt.Sprintf(`%d'`, ft)
	}
	return fmt.Sprintf(`%d'%d"`, ft, in)
}

// printResults prints a concise summary of the solution to the console
func printResults(tubing string, stockLen int, kerf float64, cuts []Cut, solution Solution) {
	totalStock := solution.NumSticks * stockLen
	efficiency := 0.0
	if totalStock > 0 {
		efficiency = float64(totalStock-solution.TotalWaste) / float64(totalStock) * 100
	}

	fmt.Println("\n--- Cut Optimization Summary ---")
	fmt.Printf("Material:      %s @ %s\n", tubing, prettyLen(stockLen))
	fmt.Printf("Sticks Needed: %d\n", solution.NumSticks)
	fmt.Printf("Efficiency:    %.1f%%\n", efficiency)
	fmt.Printf("Total Waste:   %s (avg %s per stick)\n",
		prettyLen(solution.TotalWaste),
		prettyLen(solution.TotalWaste/solution.NumSticks))
	fmt.Println("---------------------------------")

	// Group sticks into patterns for cleaner output
	patterns := groupPatterns(solution.Sticks)
	fmt.Println("\nCut Patterns (Qty | Cuts -> Waste):")
	for _, p := range patterns {
		var cutStrs []string
		for _, c := range p.Cuts {
			cutStrs = append(cutStrs, prettyLen(c.Length))
		}
		cutList := strings.Join(cutStrs, ", ")
		fmt.Printf("  %2d × | %s -> %s waste\n", p.Count, cutList, prettyLen(p.WasteLen))
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
		// Sort cuts within the stick to group patterns regardless of cut order
		sort.Slice(s.Cuts, func(i, j int) bool {
			return s.Cuts[i].Length > s.Cuts[j].Length
		})

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
		NumSticks  int
		Kerf       string
		TotalStock string
		TotalWaste string
		Efficiency string
		AvgWaste   string
		Patterns   []patternData
	}

	// Prepare pattern data for the template
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
			instr = append(instr, cutInstr{Mark: prettyLen(markAt), Len: prettyLen(c.Length)})
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
	efficiency := 0.0
	if totalStock > 0 {
		efficiency = float64(totalStock-solution.TotalWaste) / float64(totalStock) * 100
	}

	data := pageData{
		Date:       time.Now().Format("2006-01-02"),
		Tubing:     tubing,
		Stock:      prettyLen(stockLen),
		NumSticks:  solution.NumSticks,
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
        :root {
            --primary: #05445E; --accent: #189AB4; --light: #D4F1F4;
            --gray: #ECECEC; --border: #C7C7C7;
        }
        * { box-sizing: border-box; }
        body { font-family: "Segoe UI", Helvetica, Arial, sans-serif; margin: 0 auto; max-width: 960px; padding: 24px; color: #333; background: #fff; }
        h1 { color: var(--primary); margin-top: 0; }
        h2 { color: var(--accent); border-bottom: 2px solid var(--accent); padding-bottom: 4px; }
        table { width: 100%; border-collapse: collapse; margin: 16px 0; }
        th, td { padding: 10px 8px; border: 1px solid var(--border); }
        th { background: var(--gray); text-align: left; }
        tr:nth-child(even) td { background: var(--light); }
        ul { margin: 0 0 16px 20px; }
        .tag { display: inline-block; background: var(--accent); color: #fff; padding: 2px 8px; border-radius: 4px; font-size: 0.8rem; margin-left: 6px; }
    </style>
</head>
<body>
<h1>Cut Plan</h1>
<p>
    <strong>Date:</strong> {{.Date}}<br>
    <strong>Material:</strong> {{.Tubing}} @ {{.Stock}}<br>
    <strong>Kerf:</strong> {{.Kerf}}<br>
    <strong>Sticks needed:</strong> {{.NumSticks}} × {{.Stock}}
</p>
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
<h3>Pattern {{$idx | inc}}<span class="tag">Qty {{$p.Count}}</span></h3>
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
