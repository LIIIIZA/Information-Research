import json
import math
import matplotlib.pyplot as plt
import numpy as np
from collections import Counter
from datetime import datetime

class ZipfAnalyzer:
    def __init__(self):
        self.term_freq = Counter()
        self.total_terms = 0
        self.unique_terms = 0

    def load_token_freq(self, filepath):
        with open(filepath, 'r', encoding='utf-8') as f:
            for line in f:
                parts = line.strip().split('\t')
                if len(parts) == 2:
                    term, freq = parts[0], int(parts[1])
                    self.term_freq[term] = freq
                    self.total_terms += freq
        
        self.unique_terms = len(self.term_freq)
        print(f"Loaded {self.unique_terms} unique terms, {self.total_terms} total occurrences")

    def get_sorted_frequencies(self):
        sorted_freq = sorted(self.term_freq.values(), reverse=True)
        return sorted_freq

    def calculate_zipf_constant(self, sorted_freq):
        if len(sorted_freq) == 0:
            return 0
        return sorted_freq[0]

    def calculate_mandelbrot_constants(self, sorted_freq):
        if len(sorted_freq) < 3:
            return 0, 0, 0
        
        ranks = np.arange(1, min(1000, len(sorted_freq)) + 1)
        freqs = np.array(sorted_freq[:len(ranks)], dtype=float)
        
        best_error = float('inf')
        best_C, best_b, best_a = 0, 0, 0
        
        for C in [sorted_freq[0] * 0.8, sorted_freq[0], sorted_freq[0] * 1.2]:
            for b in [0, 1, 2, 3, 4, 5]:
                for a in [0.9, 1.0, 1.1, 1.2]:
                    predicted = C / ((ranks + b) ** a)
                    error = np.sum((np.log(freqs + 1) - np.log(predicted + 1)) ** 2)
                    
                    if error < best_error:
                        best_error = error
                        best_C = C
                        best_b = b
                        best_a = a
        
        return best_C, best_b, best_a

    def plot_zipf(self, output_path='zipf_law.png'):
        sorted_freq = self.get_sorted_frequencies()
        ranks = np.arange(1, len(sorted_freq) + 1)
        
        C_zipf = self.calculate_zipf_constant(sorted_freq)
        C_mand, b_mand, a_mand = self.calculate_mandelbrot_constants(sorted_freq)
        
        plt.figure(figsize=(12, 8))
        
        plt.loglog(ranks, sorted_freq, 'b-', linewidth=1.5, label='Observed frequency')
        
        zipf_curve = [C_zipf / r for r in ranks]
        plt.loglog(ranks, zipf_curve, 'r--', linewidth=2, label=f"Zipf's Law: f = {int(C_zipf)}/r")
        
        if C_mand > 0:
            mandelbrot_curve = [C_mand / ((r + b_mand) ** a_mand) for r in ranks]
            plt.loglog(ranks, mandelbrot_curve, 'g-.', linewidth=2, 
                      label=f"Mandelbrot: f = {int(C_mand)}/(r + {b_mand})^{a_mand:.1f}")
        
        plt.xlabel('Rank (log scale)', fontsize=12)
        plt.ylabel('Frequency (log scale)', fontsize=12)
        plt.title("Zipf's Law: Term Frequency Distribution", fontsize=14)
        plt.legend(fontsize=10)
        plt.grid(True, which='both', alpha=0.3)
        plt.tight_layout()
        plt.savefig(output_path, dpi=300)
        plt.close()
        
        print(f"Graph saved to {output_path}")
        
        return {
            'zipf_constant': C_zipf,
            'mandelbrot_C': C_mand,
            'mandelbrot_b': b_mand,
            'mandelbrot_a': a_mand
        }

    def calculate_statistics(self):
        sorted_freq = self.get_sorted_frequencies()
        
        top_10_count = sum(sorted_freq[:10]) if len(sorted_freq) >= 10 else sum(sorted_freq)
        top_100_count = sum(sorted_freq[:100]) if len(sorted_freq) >= 100 else sum(sorted_freq)
        top_1000_count = sum(sorted_freq[:1000]) if len(sorted_freq) >= 1000 else sum(sorted_freq)
        
        return {
            'total_terms': self.total_terms,
            'unique_terms': self.unique_terms,
            'top_10_percent': (top_10_count / self.total_terms * 100) if self.total_terms > 0 else 0,
            'top_100_percent': (top_100_count / self.total_terms * 100) if self.total_terms > 0 else 0,
            'top_1000_percent': (top_1000_count / self.total_terms * 100) if self.total_terms > 0 else 0,
            'hapax_legomena': sum(1 for f in sorted_freq if f == 1),
            'dis_legomena': sum(1 for f in sorted_freq if f == 2)
        }

def main():
    print("Zipf's Law Analysis")
    print("=" * 50)
    
    analyzer = ZipfAnalyzer()
    
    print("\nLoading token frequencies...")
    analyzer.load_token_freq('token_freq.txt')
    
    print("\nCalculating statistics...")
    stats = analyzer.calculate_statistics()
    
    print("\nCorpus Statistics:")
    print(f"Total terms: {stats['total_terms']:,}")
    print(f"Unique terms: {stats['unique_terms']:,}")
    print(f"Top 10 terms: {stats['top_10_percent']:.2f}% of all occurrences")
    print(f"Top 100 terms: {stats['top_100_percent']:.2f}% of all occurrences")
    print(f"Top 1000 terms: {stats['top_1000_percent']:.2f}% of all occurrences")
    print(f"Hapax legomena (freq=1): {stats['hapax_legomena']:,}")
    print(f"Dis legomena (freq=2): {stats['dis_legomena']:,}")
    
    print("\nPlotting Zipf's Law graph...")
    constants = analyzer.plot_zipf('zipf_law.png')
    
    print("\nFitted Constants:")
    print(f"Zipf constant (C): {int(constants['zipf_constant']):,}")
    print(f"Mandelbrot C: {int(constants['mandelbrot_C']):,}")
    print(f"Mandelbrot b: {constants['mandelbrot_b']}")
    print(f"Mandelbrot a: {constants['mandelbrot_a']:.2f}")
    
    print("\nAnalysis complete!")

if __name__ == '__main__':
    main()