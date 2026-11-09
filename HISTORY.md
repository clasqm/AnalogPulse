# History

How Futoshi Ebata's 1997 BeOS "Analog Pulse" CPU gauge became three Haiku
applications, through commit `acb954c`. The imported source is `78d8e10`;
everything after it is the port.

## 1. Getting it to compile on Haiku

The original borrowed CPU identification from `get_system_info()` fields that
Haiku has since removed, so the first port (`cc1199c`) rewrote the CPU meter
against the APIs Haiku still has. Next (`d990ac3`) came a table of modern
Intel and AMD part names, because the built-in one stops around the Pentium
era, and the gauges were renumbered from 0 to match `/dev` numbering.

## 2. Making it look right

`5e47212` was pure polish: a timer that counts the first CPU as 0, Noto fonts
instead of the BeOS defaults Haiku no longer ships, a taller panel
(`kMeterHeight` 95), a wider gap above the scale and a centred identification
line. The repository owner then tidied
the documentation (`ba4043c`, `ca8118e`), renaming the readme and deleting the
old BeIDE project files that no longer opened.

## 3. One gauge, three applications

AnalogPulse showed CPU load. Because the same dial could just as well show
memory and disk use, the CPU-specific parts of the view were split into a
shared base class, `GaugeView` (`ad0717c`): it owns the bitmap panel, the tick
table, the needle animation and the replicant archive, while a subclass only
answers which number to show, what to write over the pivot and along the top,
and which class name and signature to use. `AnalogView` keeps the CPU
behaviour,
`RamView` reports used pages as a percentage of total, and `DiskView` reports
how full the boot volume is. AnalogRAM and AnalogDisk got their own
signatures, resource files and a single-pane `GaugeWindow`, while AnalogPulse
still opens one window per CPU.

A root `CMakeLists.txt` now builds all three binaries in one pass and stamps
each with its resources (`rc`, then `xres`, then `mimeset -f`), warning rather
than failing when those tools are missing; the old makefile-engine build, which
could only ever produce AnalogPulse, was retired once CMake covered all three.

Two replicant rules cost time: `Instantiate` must be defined out of line in
the global namespace, and a binary without embedded resources still runs but
cannot be dragged out as a replicant.

## 4. The needle that kept vanishing

A screenshot from the testing machine showed both new windows with no needle
in the disk gauge and an intermittent one in the memory gauge. Both faults
came from the same place: the needle is drawn straight into the window and
wiped by copying the needle-less panel bitmap back, so any redraw of the
window left it blank until the value changed again. A reading of 0 was also
skipped, which meant the disk gauge, whose sampling was failing, never drew a
needle at all. `4676a76` gives the needle a `-1` sentinel for "nothing drawn
yet", repaints it after every redraw, reads the boot volume with
`BVolumeRoster::GetBootVolume` instead of `statvfs` on a resolved path,
labels it `/boot`, and polls ten times a second.

`acb954c` adds a temporary one-shot alert and `stderr` line naming the failing
volume call, so a gauge stuck at zero says whether the query failed or the
volume really is empty. No alert appeared on the test machine, which confirmed
the query itself, and the reporting was then removed again.

No build was ever run in the development container; there is no Haiku
toolchain in it, so compiling and testing stayed on Haiku and the work here
was checked with CMake configure runs and structural checks.
