# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

"""Plot existing benchmark observations; never runs or changes the benchmark."""

import argparse
import csv
import json
import re
from importlib import import_module
from pathlib import Path

# Optional dependencies of this manual visualization tool, not of the library.
try:
    matplotlib = import_module('matplotlib')
    matplotlib.use('Agg')
    plt = import_module('matplotlib.pyplot')
    PdfPages = import_module('matplotlib.backends.backend_pdf').PdfPages
    np = import_module('numpy')
except ModuleNotFoundError as error:
    raise SystemExit('Install matplotlib in a local environment before plotting.') from error

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('results', type=Path)
parser.add_argument('output', type=Path)
args = parser.parse_args()
data = json.loads((args.results / 'summary.json').read_text())
groups = data['groups']
assert len(groups) == 102 and data['cases'] == 442
args.output.mkdir(parents=True, exist_ok=True)
limits = [1, 10, 100, 1000, 5000, 10000]
x = np.arange(len(limits))
colors = {'jspi': '#2266A5', 'asyncify': '#C25C16', 'js': '#29805F'}
plt.rcParams.update({'font.family': 'DejaVu Sans', 'font.size': 11,
                     'axes.titlesize': 14, 'axes.labelsize': 11,
                     'axes.spines.top': False, 'axes.spines.right': False,
                     'figure.facecolor': 'white', 'axes.facecolor': 'white',
                     'svg.fonttype': 'none', 'pdf.fonttype': 42,
                     'savefig.facecolor': 'white'})
pdf = PdfPages(args.output / 'yield-benchmark-charts.pdf')


def get(group, variant, mode='direct', limit=1, trace=0, control=0):
    match = dict(group=group, variant=variant, mode=mode, limit=limit,
                 trace=trace, control=control)
    found = [g for g in groups if all(g[k] == v for k, v in match.items())]
    assert len(found) == 1, match
    return found[0]


def frame(ax, title, ylabel=None, settings=False):
    ax.set_title(title, loc='left', pad=13, fontweight='bold')
    if ylabel:
        ax.set_ylabel(ylabel)
    ax.grid(axis='y', color='#E2E6EA', linewidth=0.7)
    ax.set_axisbelow(True)
    if settings:
        ax.set_xticks(x, ['1', '10', '100', '1,000', '5,000', '10,000'])
        ax.set_xlabel('Handlers per turn (tested settings)')


def heading(fig, title, subtitle):
    fig.suptitle(title, x=0.06, y=0.975, ha='left', fontsize=21, fontweight='bold')
    fig.text(0.06, 0.925, subtitle, fontsize=11, color='#475569')


def save(fig, name, note):
    fig.text(0.06, 0.025, note, fontsize=9, color='#475569', va='bottom')
    fig.canvas.draw()
    renderer = fig.canvas.get_renderer()
    width, height = fig.canvas.get_width_height()
    for text in fig.texts:
        box = text.get_window_extent(renderer)
        assert box.x0 >= 0 and box.y0 >= 0 and box.x1 <= width and box.y1 <= height
    fig.savefig(args.output / (name + '.png'), dpi=150)
    svg_path = args.output / (name + '.svg')
    fig.savefig(svg_path)
    # Namespace SVG IDs so multiple inline figures can share one document.
    svg = svg_path.read_text()
    svg = re.sub(r'id="([^"]+)"', lambda m: f'id="{name}-{m[1]}"', svg)
    svg = re.sub(r'url\(#([^)]+)\)', lambda m: f'url(#{name}-{m[1]})', svg)
    svg = re.sub(r'xlink:href="#([^"]+)"', lambda m: f'xlink:href="#{name}-{m[1]}"', svg)
    svg_path.write_text(svg)
    pdf.savefig(fig)
    plt.close(fig)


# Figure 1: actual gaps and their explicitly derived per-handler amortization.
fig, axs = plt.subplots(1, 2, figsize=(16, 8))
fig.subplots_adjust(left=0.07, right=0.97, top=0.81, bottom=0.16, wspace=0.25)
heading(fig, 'Yield latency and turn limits',
        'Release -O3 | Exceptions enabled | Isolated page | Seven repetitions per setting')
for variant in ('jspi', 'asyncify'):
    records = [get('primary', variant, 'chain', n) for n in limits]
    means = np.array([g['mean_us']['median'] for g in records])
    low = means - np.array([g['mean_us']['min'] for g in records])
    high = np.array([g['mean_us']['max'] for g in records]) - means
    axs[0].errorbar(x, means, yerr=[low, high], color=colors[variant],
                   marker='o' if variant == 'jspi' else 's', capsize=4,
                   linewidth=2, label=variant.upper())
    amortized = means * 1000 / np.array(limits)
    axs[1].plot(x, amortized, color=colors[variant], linewidth=2,
                marker='o' if variant == 'jspi' else 's', label=variant.upper())
    for i, value in enumerate(means):
        anchor = records[i]['mean_us']['max' if variant == 'asyncify' else 'min']
        axs[0].annotate(f'{value:.2f}', (i, anchor), xytext=(0, 8 if variant == 'asyncify'
                         else -18), textcoords='offset points', ha='center',
                         color=colors[variant], fontsize=10)
frame(axs[0], 'Measured boundary gap: excludes callback batch work',
      'Microseconds per yield gap', True)
axs[0].set_ylim(0, 8)
frame(axs[1], 'Derived scheduling cost per callback',
      'Nanoseconds per callback (log scale)', True)
axs[1].set_yscale('log')
axs[0].legend(loc='upper left', frameon=False)
axs[1].legend(loc='upper right', frameon=False)
save(fig, '01-yield-latency',
     'Dots: median of per-run means. Error bars: range of run means, not confidence intervals.\n'
     'Amortization = measured gap / turn limit. '
     'Gaps include small dispatcher return/reentry costs.')

# Figure 2: every direct control, and all traced direct paths.
fig, axs = plt.subplots(1, 2, figsize=(17, 11))
fig.subplots_adjust(left=0.20, right=0.97, top=0.85, bottom=0.24, wspace=0.65)
heading(fig, 'Direct round trips and observed phases',
        'Same Promise / MessageChannel mechanism | '
        'Untraced and traced measurements kept separate')
direct = []
for group, suffix in [('primary', 'EH on'), ('no-exceptions', 'EH off'),
                       ('ordinary-page', 'ordinary page')]:
    for variant in ('jspi', 'asyncify'):
        for trace in (0, 1):
            direct.append((f'{variant.upper()} / {suffix}' + (' / traced' if trace else ''),
                           get(group, variant, trace=trace)))
for trace in (0, 1):
    direct.append(('JavaScript only' + (' / traced' if trace else ''),
                   get('js-control', 'js', 'js-direct', trace=trace)))
for i, (label, g) in enumerate(direct):
    m = g['mean_us']
    axs[0].barh(i, m['median'], color=colors[g['variant']],
                alpha=0.5 if g['trace'] else 1, height=0.65)
    axs[0].errorbar(m['median'], i, xerr=[[m['median'] - m['min']],
                    [m['max'] - m['median']]], color='#283544', capsize=3, fmt='none')
    axs[0].text(m['max'] + 0.07, i, f"{m['median']:.2f}", va='center', fontsize=10)
axs[0].set_yticks(range(len(direct)), [label for label, _ in direct], fontsize=10)
axs[0].invert_yaxis()
axs[0].set_xlim(0, 6.4)
axs[0].set_xlabel('Microseconds per round trip')
frame(axs[0], 'Direct latency: median of run means')
parts = ['entry', 'channel_create', 'handler_setup', 'post_message',
         'post_to_delivery', 'message_handler', 'resume']
part_labels = ['Caller entry', 'Channel construction', 'Handler + observer setup',
               'postMessage', 'Post return to delivery', 'Close port + resolve', 'Resume']
part_colors = ['#5965A7', '#2B79A1', '#319B9A', '#A3BF69', '#E6B651', '#DA8150', '#AE5B79']
traced = [(label.replace(' / traced', ''), g) for label, g in direct if g['trace']]
left = np.zeros(len(traced))
for part, label, color in zip(parts, part_labels, part_colors):
    values = np.array([g['component_means_us'][part] for _, g in traced])
    axs[1].barh(range(len(traced)), values, left=left, label=label, color=color, height=0.6)
    left += values
for i, value in enumerate(left):
    axs[1].text(value + 0.06, i, f'{value:.2f}', va='center', fontsize=10)
axs[1].set_yticks(range(len(traced)), [label for label, _ in traced], fontsize=10)
axs[1].invert_yaxis()
axs[1].set_xlim(0, 6.5)
axs[1].set_xlabel('Microseconds, arithmetic means of traced runs')
frame(axs[1], 'Instrumented phases: means add to the total')
axs[1].legend(loc='upper center', bbox_to_anchor=(0.42, -0.12), ncol=2,
              frameon=False, fontsize=10)
save(fig, '02-direct-and-phases',
     'Primary and JS-only controls: 7 runs; EH-off and ordinary-page controls: 3 runs.\n'
     'The observer adds cost. Post-to-delivery includes suspension and scheduling, '
     'not pure queue wait.\n'
     'Sub-resolution phase values are aggregate estimates; individual clock steps are 5 or 100 us.')

# Figure 3: all chain comparison groups and paired whole-run controls.
fig, axs = plt.subplots(2, 2, figsize=(16, 12))
fig.subplots_adjust(left=0.08, right=0.97, top=0.85, bottom=0.13, hspace=0.55, wspace=0.22)
heading(fig, 'Chain controls and whole-program comparisons',
        'All six turn limits | Same callback and timestamp counts within each control pair')
styles = [('primary', 'EH on, untraced', '#2266A5', 'o'),
          ('no-exceptions', 'EH off, untraced', '#29805F', 's'),
          ('work-64', '64 source-level LCG updates', '#C25C16', '^'),
          ('chain-trace', 'EH on, traced', '#8960A5', 'D')]
for column, variant in enumerate(('jspi', 'asyncify')):
    for group, label, color, marker in styles:
        records = [get(group, variant, 'chain', n, trace=int(group == 'chain-trace'))
                   for n in limits]
        axs[0, column].plot(x, [g['mean_us']['median'] for g in records],
                            color=color, marker=marker, linewidth=1.8, label=label)
        if group != 'chain-trace':
            ratios = [g['paired_slowdown'] for g in records]
            m = np.array([r['median'] for r in ratios])
            axs[1, column].errorbar(x, m,
                yerr=[m - np.array([r['min'] for r in ratios]),
                      np.array([r['max'] for r in ratios]) - m],
                color=color, marker=marker, capsize=3, linewidth=1.8, label=label)
    frame(axs[0, column], variant.upper() + ': measured yield boundary gaps',
          'Microseconds (median of run means)', True)
    axs[0, column].set_ylim(0, 10)
    axs[0, column].legend(frameon=False, fontsize=9, loc='upper left')
    frame(axs[1, column], variant.upper() + ': elapsed-time ratio vs no periodic yields',
          'Whole-run ratio (log scale)', True)
    axs[1, column].set_yscale('log')
    axs[1, column].axhline(1, color='#687483', linestyle='--', linewidth=1)
    axs[1, column].set_ylim(0.5, 30)
save(fig, '03-chain-controls',
     'Ratios below 1 are observed whole-program outcomes, not negative yield costs. '
     'Runtime effects confound subtraction.\n'
     'Primary: 7 runs; other groups: 3. Error bars show the range of paired ratios.\n'
     'LCG updates are source-level work, not a guarantee of 64 instructions after optimization.')

# Figure 4: tails, clock read controls, resolution and quantized zeros.
fig, axs = plt.subplots(2, 2, figsize=(16, 12))
fig.subplots_adjust(left=0.10, right=0.97, top=0.85, bottom=0.12, hspace=0.65, wspace=0.42)
heading(fig, 'Tails, clocks and measurement resolution',
        'Extreme observations are retained | '
        'A quantized zero is not proof of a zero-cost operation')
for variant in ('jspi', 'asyncify'):
    records = [get('primary', variant, 'chain', n) for n in limits]
    for metric, aggregate, style, suffix in [('max_us', 'max', '-', 'maximum'),
                                            ('p99_us', 'median', '--', 'p99')]:
        axs[0, 0].plot(x, [g[metric][aggregate] for g in records], linestyle=style,
                       marker='o' if variant == 'jspi' else 's', color=colors[variant],
                       label=variant.upper() + ' ' + suffix)
frame(axs[0, 0], 'Chain tails: p99 and largest observed gap',
      'Microseconds (log scale)', True)
axs[0, 0].set_yscale('log')
axs[0, 0].legend(frameon=False, fontsize=9)
metrics = [('direct', 'js_to_cpp_sync_ns', 'JS to Wasm increment loop', 1),
           ('direct', 'performance_clock_loop_ns', 'JS monotonic clock study', 1),
           ('direct', 'cpp_clock_loop_ns', 'Wasm clock via JS study', 1),
           ('direct', 'wall_clock_loop_ns', 'JS wall-clock study', 1),
           ('clock', 'mean_us', 'Adjacent Wasm timestamps', 1000),
           ('sync', 'mean_us', 'Wasm to JS + timestamps', 1000)]
y = np.arange(len(metrics))
for offset, variant in [(-0.18, 'jspi'), (0.18, 'asyncify')]:
    values = [get('primary', variant, mode)[metric]['median'] * factor
              for mode, metric, _, factor in metrics]
    axs[0, 1].barh(y + offset, values, height=0.32, color=colors[variant],
                   label=variant.upper())
axs[0, 1].set_yticks(y, [label for _, _, label, _ in metrics], fontsize=9)
axs[0, 1].invert_yaxis()
axs[0, 1].set_xscale('log')
axs[0, 1].set_xlabel('Nanoseconds per iteration (log scale)')
frame(axs[0, 1], 'Synchronous and clock-read controls')
axs[0, 1].legend(frameon=False, fontsize=9)
axs[1, 0].bar(['Isolated monotonic', 'Ordinary monotonic', 'Wall clock'],
              [5, 100, 1000], color=['#2266A5', '#8960A5', '#687483'], width=0.55)
axs[1, 0].set_yscale('log')
for i, value in enumerate([5, 100, 1000]):
    axs[1, 0].text(i, value * 1.15, f'{value:,} us', ha='center')
axs[1, 0].set_ylim(1, 3000)
frame(axs[1, 0], 'Observed positive clock steps', 'Microseconds (log scale)')
for offset, variant in [(-0.18, 'jspi'), (0.18, 'asyncify')]:
    values = [get(group, variant)['zero_percent']['median']
              for group in ['primary', 'ordinary-page']]
    axs[1, 1].bar(np.arange(2) + offset, values, width=0.32,
                  color=colors[variant], label=variant.upper())
    for i, value in enumerate(values):
        axs[1, 1].text(i + offset, value + 2, f'{value:.1f}%', ha='center', fontsize=10)
axs[1, 1].set_xticks([0, 1], ['Isolated page', 'Ordinary page'])
axs[1, 1].set_ylim(0, 108)
frame(axs[1, 1], 'Direct samples recorded as zero', 'Percent of samples')
axs[1, 1].legend(frameon=False, loc='center left')
save(fig, '04-tails-and-clocks',
     'Clock studies include loop overhead; the trivial synchronous export may be inlined. '
     'Wall-clock step: where observed.\n'
     'Wasm and JS monotonic clocks share performance.now(). '
     'Maximums are observations, not guaranteed latency bounds.')
pdf.close()


# Preserve every aggregate field, including repeat ranges and traced components.
def flatten(value, prefix=''):
    result = {}
    for key, item in value.items():
        label = prefix + key
        if isinstance(item, dict):
            result.update(flatten(item, label + '.'))
        else:
            result[label] = item
    return result


rows = [flatten(g) for g in groups]
fields = list(dict.fromkeys(key for row in rows for key in row))
with (args.output / 'all-configurations.csv').open('w', newline='') as stream:
    writer = csv.DictWriter(stream, fieldnames=fields, lineterminator='\n')
    writer.writeheader()
    writer.writerows(rows)
manifest = dict(source='summary.json', configurations=len(rows), runs=data['cases'],
                matplotlib=matplotlib.__version__,
                charts=['01-yield-latency', '02-direct-and-phases',
                        '03-chain-controls', '04-tails-and-clocks'])
(args.output / 'figures.json').write_text(json.dumps(manifest, indent=2) + '\n')
print(f'Created 4 figures (PNG/SVG), a 4-page PDF and {len(rows)} complete aggregate rows.')
