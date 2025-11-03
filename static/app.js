// Utility Functions (shared across the app)
const utils = {
    parseFraction(s) {
        s = s.trim();
        if (!s) return 0;

        if (s.includes('/')) {
            const match = s.match(/^\s*(\d+(?:\.\d+)?)\s*\/\s*(\d+(?:\.\d+)?)\s*$/);
            if (match) {
                const num = parseFloat(match[1]);
                const den = parseFloat(match[2]);
                if (den !== 0) return num / den;
            }
        } else {
            const val = parseFloat(s);
            if (!isNaN(val)) return val;
        }
        return 0;
    },

    parseAdvancedLength(s) {
        let input = s.trim();
        if (!input) return 0;

        // Normalize smart quotes to straight quotes
        input = input.replace(/[\u2018\u2019]/g, "'");

        let totalInches = 0;

        const feetPos = input.indexOf("'");
        if (feetPos !== -1) {
            const feetStr = input.substring(0, feetPos);
            totalInches += this.parseFraction(feetStr) * 12;
            input = input.substring(feetPos + 1).trim();
        }

        if (!input) return totalInches;

        if (input.endsWith('"')) {
            input = input.slice(0, -1).trim();
        }

        const inchMarkPos = input.indexOf('"');
        if (inchMarkPos !== -1) {
            const beforeMark = input.substring(0, inchMarkPos);
            totalInches += this.parseFraction(beforeMark);

            const afterMark = input.substring(inchMarkPos + 1).trim();
            if (afterMark) {
                totalInches += this.parseFraction(afterMark);
            }
        } else {
            let lastSpace = -1;
            for (let i = 0; i < input.length; i++) {
                if (/\s/.test(input[i])) {
                    const remaining = input.substring(i + 1).trim();
                    if (remaining.includes('/')) {
                        lastSpace = i;
                    }
                }
            }

            if (lastSpace !== -1) {
                const wholePart = input.substring(0, lastSpace);
                const fracPart = input.substring(lastSpace + 1);

                const parsedWhole = this.parseFraction(wholePart);
                const parsedFrac = this.parseFraction(fracPart);

                if (!isNaN(parsedWhole) && !isNaN(parsedFrac)) {
                    totalInches += parsedWhole;
                    totalInches += parsedFrac;
                } else {
                    totalInches += this.parseFraction(input);
                }
            } else {
                totalInches += this.parseFraction(input);
            }
        }

        return totalInches;
    },

    prettyLen(inches) {
        if (Math.abs(inches) < 1/64) return '0"';

        inches = Math.round(inches * 32) / 32;

        const feet = Math.floor(inches / 12);
        let remaining = inches - (feet * 12);

        if (remaining >= 12 - 1/64) {
            return (feet + 1) + "'";
        }

        const wholeInches = Math.floor(remaining);
        const fraction = remaining - wholeInches;

        let result = '';
        if (feet > 0) result += feet + "'";
        if (feet > 0 && (wholeInches > 0 || fraction > 1/64)) result += " ";

        if (wholeInches > 0) result += wholeInches;

        if (fraction > 1/64) {
            if (wholeInches > 0) result += " ";
            let denominator = 32;
            let numerator = Math.round(fraction * denominator);

            const gcd = (a, b) => b === 0 ? a : gcd(b, a % b);
            const common = gcd(numerator, denominator);
            numerator /= common;
            denominator /= common;

            result += numerator + "/" + denominator;
        }

        if (!feet && (wholeInches > 0 || fraction > 1/64)) result += '"';
        else if (feet > 0 && !(wholeInches > 0 || fraction > 1/64)) { }
        else if (feet > 0 && (wholeInches > 0 || fraction > 1/64)) result += '"';

        return result || '0"';
    }
};

// Main Alpine.js application
document.addEventListener('alpine:init', () => {
    Alpine.data('nestingApp', () => ({
        // State
        jobName: 'Cut Plan',
        materialType: 'Standard Material',
        stockLength: "24'",
        kerf: '1/8',
        cuts: [],
        cutLength: '',
        cutQuantity: 1,
        errorMsg: '',
        loading: false,
        results: null,
        showModal: false,
        globalColorMap: {},
        nextColorIndex: 1,

        // Computed previews
        get stockPreview() {
            if (!this.stockLength) return { show: false };
            const value = utils.parseAdvancedLength(this.stockLength);
            return {
                show: true,
                valid: value > 0,
                text: value > 0 ? utils.prettyLen(value) : 'Invalid format'
            };
        },

        get kerfPreview() {
            if (!this.kerf) return { show: false };
            const value = utils.parseFraction(this.kerf);
            const decimal = value.toFixed(4).replace(/\.?0+$/, '');
            let pretty = utils.prettyLen(value);
            if (pretty.startsWith('0\'')) pretty = pretty.substring(2).trim();
            if (pretty.startsWith('0"')) pretty = pretty.substring(2).trim();
            if (pretty === '') pretty = '0';

            return {
                show: true,
                valid: value > 0,
                text: value > 0 ? `${pretty} (${decimal}")` : 'Invalid format'
            };
        },

        get cutPreview() {
            if (!this.cutLength) return { show: false };
            const value = utils.parseAdvancedLength(this.cutLength);
            return {
                show: true,
                valid: value > 0,
                text: value > 0 ? utils.prettyLen(value) : 'Invalid format'
            };
        },

        get hasCuts() {
            return this.cuts.length > 0;
        },

        // Methods
        addCut() {
            if (!this.cutLength.trim()) {
                this.showError('Please enter a cut length');
                return;
            }

            const inches = utils.parseAdvancedLength(this.cutLength);
            if (inches <= 0) {
                this.showError('Invalid length format');
                return;
            }

            const normalizedLength = utils.prettyLen(inches);
            this.cuts.push({
                length: normalizedLength,
                quantity: parseInt(this.cutQuantity) || 1
            });

            this.cutLength = '';
            this.cutQuantity = 1;
        },

        removeCut(index) {
            this.cuts.splice(index, 1);
        },

        showError(message) {
            this.errorMsg = message;
            setTimeout(() => {
                this.errorMsg = '';
            }, 5000);
        },

        getCutColorClass(cutLength) {
            const key = cutLength.toFixed(4);
            if (!this.globalColorMap[key]) {
                this.globalColorMap[key] = `color-${this.nextColorIndex++}`;
                if (this.nextColorIndex > 6) this.nextColorIndex = 1;
            }
            return this.globalColorMap[key];
        },

        async optimize() {
            if (this.cuts.length === 0) {
                this.showError('Please add at least one cut');
                return;
            }

            this.loading = true;
            this.results = null;

            const data = {
                jobName: this.jobName || 'Cut Plan',
                materialType: this.materialType || 'Standard Material',
                stockLength: this.stockLength,
                kerf: this.kerf,
                cuts: this.cuts
            };

            try {
                const response = await fetch('/api/optimize', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(data)
                });

                const result = await response.json();

                if (!response.ok) {
                    throw new Error(result.error || 'Optimization failed');
                }

                this.results = result;

                // Scroll to results
                this.$nextTick(() => {
                    const resultsEl = this.$refs.results;
                    if (resultsEl) {
                        const y = resultsEl.getBoundingClientRect().top + window.pageYOffset - 20;
                        window.scrollTo({ top: y < 0 ? 0 : y, behavior: 'smooth' });
                    }
                });
            } catch (error) {
                this.showError(error.message);
            } finally {
                this.loading = false;
            }
        },

        formatLength(inches) {
            return utils.prettyLen(inches);
        },

        formatDate(date) {
            return new Date(date).toLocaleDateString();
        },

        printResults() {
            window.print();
        },

        // Keyboard handlers
        handleCutKeypress(event) {
            if (event.key === 'Enter') {
                this.addCut();
            }
        }
    }));
});
