#!/usr/bin/env python3
"""
Generate performance charts for mirror_hash blog post.
Creates compelling visualizations that support the blog narrative.
"""

import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import numpy as np
import os

# Set style for clean, professional look
plt.style.use('seaborn-v0_8-whitegrid')
plt.rcParams['font.family'] = 'sans-serif'
plt.rcParams['font.size'] = 11
plt.rcParams['axes.labelsize'] = 12
plt.rcParams['axes.titlesize'] = 14
plt.rcParams['legend.fontsize'] = 10
plt.rcParams['figure.facecolor'] = 'white'

# Output directory
OUTPUT_DIR = "/Users/random_person/franciscothiesen.github.io/images"
os.makedirs(OUTPUT_DIR, exist_ok=True)

# Benchmark data from actual runs (M3 Max Pro MacBook, mirror_hash v2.1)
# Size in bytes, times in nanoseconds
sizes = [8, 16, 24, 32, 48, 64, 96, 128, 192, 256, 384, 512, 768, 1024, 2048, 4096, 8192]

# mirror_hash v2.1 times (optimized single-state AES with overlapping read)
mirror_ns = [1.73, 1.72, 1.88, 1.88, 1.97, 2.41, 3.18, 3.50, 3.75, 4.01, 5.07, 5.88, 7.76, 9.62, 17.53, 31.95, 61.26]

# rapidhash times
rapid_ns = [1.34, 1.34, 1.88, 1.87, 2.15, 2.50, 3.57, 4.14, 6.27, 6.74, 9.23, 11.80, 18.73, 20.51, 40.44, 78.22, 148.47]

# GxHash times (from gxhash comparison benchmark - fewer data points, interpolated)
gx_sizes = [8, 64, 128, 256, 512, 1024, 4096, 8192]
gx_ns = [2.21, 4.02, 3.48, 4.29, 6.17, 9.91, 38.60, 79.42]

# Calculate throughput in GB/s
def calc_throughput(sizes, times_ns):
    return [s / t for s, t in zip(sizes, times_ns)]

mirror_gbps = calc_throughput(sizes, mirror_ns)
rapid_gbps = calc_throughput(sizes, rapid_ns)
gx_gbps = calc_throughput(gx_sizes, gx_ns)

# Colors
MIRROR_COLOR = '#2ecc71'  # Green
RAPID_COLOR = '#3498db'   # Blue
GX_COLOR = '#e74c3c'      # Red
ZONE_COLORS = {
    'rapid_wins': '#ffcccc',
    'mirror_wins': '#ccffcc',
    'even': '#ffffcc'
}

# =============================================================================
# Chart 1: Throughput Comparison (Log-Log Scale)
# =============================================================================
def create_throughput_chart():
    fig, ax = plt.subplots(figsize=(10, 6))

    ax.loglog(sizes, mirror_gbps, 'o-', color=MIRROR_COLOR, linewidth=2.5,
              markersize=8, label='mirror_hash', zorder=3)
    ax.loglog(sizes, rapid_gbps, 's--', color=RAPID_COLOR, linewidth=2,
              markersize=6, label='rapidhash', zorder=2)
    ax.loglog(gx_sizes, gx_gbps, '^:', color=GX_COLOR, linewidth=2,
              markersize=6, label='GxHash', zorder=2)

    # Add zone annotations
    ax.axvspan(8, 16, alpha=0.15, color='blue', label='_rapidhash territory')
    ax.axvspan(17, 48, alpha=0.15, color='red', label='_transition zone')
    ax.axvspan(64, 8192, alpha=0.08, color='green', label='_mirror_hash territory')

    # Add text annotations for zones
    ax.text(11, 2, 'Small\n(~even)', fontsize=9, ha='center', va='bottom', alpha=0.7)
    ax.text(32, 3, 'Transition\n(rapidhash\nwins)', fontsize=9, ha='center', va='bottom',
            color='darkred', alpha=0.8)
    ax.text(400, 15, 'AES Territory\n(mirror_hash wins)', fontsize=10, ha='center',
            va='bottom', color='darkgreen', alpha=0.8)

    ax.set_xlabel('Input Size (bytes)')
    ax.set_ylabel('Throughput (GB/s)')
    ax.set_title('Hash Function Throughput: mirror_hash vs rapidhash vs GxHash\n(M3 Max Pro MacBook, ARM64 with AES)')
    ax.legend(loc='upper left')
    ax.set_xlim(6, 10000)
    ax.set_ylim(1, 200)

    # Custom x-axis labels
    ax.set_xticks([8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192])
    ax.set_xticklabels(['8B', '16B', '32B', '64B', '128B', '256B', '512B', '1KB', '2KB', '4KB', '8KB'])

    plt.tight_layout()
    plt.savefig(f'{OUTPUT_DIR}/mirror-hash-throughput.png', dpi=150, bbox_inches='tight')
    plt.savefig(f'{OUTPUT_DIR}/mirror-hash-throughput.svg', bbox_inches='tight')
    print(f"Created: {OUTPUT_DIR}/mirror-hash-throughput.png")
    plt.close()

# =============================================================================
# Chart 2: Speedup Bar Chart (Key Sizes)
# =============================================================================
def create_speedup_chart():
    # Select key sizes for bar chart
    key_sizes = [8, 32, 64, 128, 256, 512, 1024, 4096, 8192]
    key_labels = ['8B', '32B', '64B', '128B', '256B', '512B', '1KB', '4KB', '8KB']

    # Calculate speedup (positive = mirror wins, negative = rapidhash wins)
    speedups = []
    for sz in key_sizes:
        idx = sizes.index(sz)
        speedup = (rapid_ns[idx] / mirror_ns[idx] - 1) * 100
        speedups.append(speedup)

    fig, ax = plt.subplots(figsize=(10, 5))

    colors = [MIRROR_COLOR if s > 0 else RAPID_COLOR for s in speedups]
    bars = ax.bar(key_labels, speedups, color=colors, edgecolor='white', linewidth=1.5)

    # Add value labels on bars
    for bar, speedup in zip(bars, speedups):
        height = bar.get_height()
        label = f'+{speedup:.0f}%' if speedup > 0 else f'{speedup:.0f}%'
        va = 'bottom' if height > 0 else 'top'
        offset = 3 if height > 0 else -3
        ax.annotate(label, xy=(bar.get_x() + bar.get_width()/2, height),
                   xytext=(0, offset), textcoords='offset points',
                   ha='center', va=va, fontsize=10, fontweight='bold')

    ax.axhline(y=0, color='black', linestyle='-', linewidth=0.8)
    ax.set_ylabel('Speedup (%)')
    ax.set_xlabel('Input Size')
    ax.set_title('mirror_hash Speedup vs rapidhash\n(Positive = mirror_hash faster, Negative = rapidhash faster)')

    # Add legend
    mirror_patch = mpatches.Patch(color=MIRROR_COLOR, label='mirror_hash wins')
    rapid_patch = mpatches.Patch(color=RAPID_COLOR, label='rapidhash wins')
    ax.legend(handles=[mirror_patch, rapid_patch], loc='upper right')

    ax.set_ylim(-80, 200)

    plt.tight_layout()
    plt.savefig(f'{OUTPUT_DIR}/mirror-hash-speedup.png', dpi=150, bbox_inches='tight')
    plt.savefig(f'{OUTPUT_DIR}/mirror-hash-speedup.svg', bbox_inches='tight')
    print(f"Created: {OUTPUT_DIR}/mirror-hash-speedup.png")
    plt.close()

# =============================================================================
# Chart 3: Latency Comparison (Linear Scale, Focused)
# =============================================================================
def create_latency_chart():
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12, 5))

    # Left panel: Small inputs (8-64 bytes)
    small_sizes = [8, 16, 24, 32, 48, 64]
    small_idx = [sizes.index(s) for s in small_sizes]
    small_mirror = [mirror_ns[i] for i in small_idx]
    small_rapid = [rapid_ns[i] for i in small_idx]

    x = np.arange(len(small_sizes))
    width = 0.35

    bars1 = ax1.bar(x - width/2, small_rapid, width, label='rapidhash', color=RAPID_COLOR)
    bars2 = ax1.bar(x + width/2, small_mirror, width, label='mirror_hash', color=MIRROR_COLOR)

    ax1.set_ylabel('Latency (ns)')
    ax1.set_xlabel('Input Size')
    ax1.set_title('Small Inputs: The Transition Zone')
    ax1.set_xticks(x)
    ax1.set_xticklabels(['8B', '16B', '24B', '32B', '48B', '64B'])
    ax1.legend()

    # Add annotation for transition zone
    ax1.axvspan(1.5, 4.5, alpha=0.2, color='red')
    ax1.text(3, 3.8, 'Transition Zone\n(17-48B)', ha='center', fontsize=9,
             color='darkred', style='italic')

    # Right panel: Large inputs (128B - 8KB)
    large_sizes = [128, 256, 512, 1024, 2048, 4096, 8192]
    large_idx = [sizes.index(s) for s in large_sizes]
    large_mirror = [mirror_ns[i] for i in large_idx]
    large_rapid = [rapid_ns[i] for i in large_idx]

    x = np.arange(len(large_sizes))

    bars1 = ax2.bar(x - width/2, large_rapid, width, label='rapidhash', color=RAPID_COLOR)
    bars2 = ax2.bar(x + width/2, large_mirror, width, label='mirror_hash', color=MIRROR_COLOR)

    ax2.set_ylabel('Latency (ns)')
    ax2.set_xlabel('Input Size')
    ax2.set_title('Large Inputs: AES Acceleration Wins')
    ax2.set_xticks(x)
    ax2.set_xticklabels(['128B', '256B', '512B', '1KB', '2KB', '4KB', '8KB'])
    ax2.legend()

    # Add speedup annotations on large chart
    for i, (r, m) in enumerate(zip(large_rapid, large_mirror)):
        speedup = (r/m - 1) * 100
        ax2.annotate(f'+{speedup:.0f}%', xy=(i + width/2, m),
                    xytext=(0, 5), textcoords='offset points',
                    ha='center', va='bottom', fontsize=8, color='darkgreen')

    plt.tight_layout()
    plt.savefig(f'{OUTPUT_DIR}/mirror-hash-latency.png', dpi=150, bbox_inches='tight')
    plt.savefig(f'{OUTPUT_DIR}/mirror-hash-latency.svg', bbox_inches='tight')
    print(f"Created: {OUTPUT_DIR}/mirror-hash-latency.png")
    plt.close()

# =============================================================================
# Chart 4: The "Why AES Wins" Instruction Count Visualization
# =============================================================================
def create_instruction_chart():
    fig, ax = plt.subplots(figsize=(8, 5))

    categories = ['rapidhash\n(128-bit multiply)', 'mirror_hash\n(AES round)']
    instructions = [3, 2]
    cycles = [4.5, 2]  # Approximate cycles
    bytes_processed = [16, 16]

    x = np.arange(len(categories))
    width = 0.3

    bars1 = ax.bar(x - width, instructions, width, label='Instructions', color='#3498db')
    bars2 = ax.bar(x, cycles, width, label='Cycles (approx)', color='#e74c3c')
    bars3 = ax.bar(x + width, [c/16 for c in cycles], width, label='Cycles/Byte', color='#2ecc71')

    ax.set_ylabel('Count')
    ax.set_title('Why AES is Faster: Instruction Efficiency\n(per 16 bytes of mixing)')
    ax.set_xticks(x)
    ax.set_xticklabels(categories)
    ax.legend()

    # Add value labels
    for bars in [bars1, bars2, bars3]:
        for bar in bars:
            height = bar.get_height()
            ax.annotate(f'{height:.2f}' if height < 1 else f'{height:.1f}',
                       xy=(bar.get_x() + bar.get_width()/2, height),
                       xytext=(0, 3), textcoords='offset points',
                       ha='center', va='bottom', fontsize=9)

    # Add explanatory text
    ax.text(0.5, -0.15,
            'AES uses dedicated silicon: AESE+AESMC fuse into ~2 cycles on Apple Silicon',
            transform=ax.transAxes, ha='center', fontsize=9, style='italic', alpha=0.7)

    plt.tight_layout()
    plt.savefig(f'{OUTPUT_DIR}/mirror-hash-instructions.png', dpi=150, bbox_inches='tight')
    plt.savefig(f'{OUTPUT_DIR}/mirror-hash-instructions.svg', bbox_inches='tight')
    print(f"Created: {OUTPUT_DIR}/mirror-hash-instructions.png")
    plt.close()

# =============================================================================
# Generate all charts
# =============================================================================
if __name__ == '__main__':
    print("Generating blog charts...")
    create_throughput_chart()
    create_speedup_chart()
    create_latency_chart()
    create_instruction_chart()
    print("\nAll charts generated successfully!")
    print(f"Output directory: {OUTPUT_DIR}")
