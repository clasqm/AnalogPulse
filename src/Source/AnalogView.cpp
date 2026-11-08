#include "AnalogView.h"

#include <Archivable.h>
#include <AppFileInfo.h>
#include <Message.h>
#include <OS.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Modern Intel and AMD model names only exist in the brand string of the
// processor itself (CPUID leaves 0x80000002 - 0x80000004).  GCC provides
// <cpuid.h> for that; without the header we keep the vendor name instead.
#if (defined(__i386__) || defined(__x86_64__)) && __has_include(<cpuid.h>)
#	include <cpuid.h>
#	define AP_HAVE_CPUID 1
#endif

#define kSignature	"application/x-vnd.takamatsu-analog-pulse"

// Haiku's system_info no longer carries the per-CPU cpu_infos[] array that
// BeOS had. Per-CPU busy time is fetched with get_cpu_info() instead.
static double
cpuActiveTime(int cpu)
{
	if (cpu < 0)
		return 0;

	cpu_info	info[1];
	if (get_cpu_info((uint32)cpu, 1, info) != B_OK)
		return 0;

	return (double)info[0].active_time;
}

// Haiku dropped system_info::cpu_type, ::cpu_clock_speed and ::cpu_infos.
// CPU identification and clock speed now come from the CPU topology API.
static const char*
cpuPlatformName(enum cpu_platform platform)
{
	switch(platform)
	{
		case B_CPU_x86:		return "x86";
		case B_CPU_x86_64:	return "x86_64";
		case B_CPU_M68K:	return "m68k";
		case B_CPU_ARM:		return "ARM";
		case B_CPU_ARM_64:	return "ARM 64";
		case B_CPU_ALPHA:	return "Alpha";
		case B_CPU_MIPS:	return "MIPS";
		case B_CPU_SH:		return "SH";
		case B_CPU_SPARC:	return "SPARC";
		case B_CPU_RISC_V:	return "RISC-V";
		default:			return "unknown";
	}
}

static const char*
cpuVendorName(enum cpu_vendor vendor)
{
	switch(vendor)
	{
		case B_CPU_VENDOR_AMD:		return "AMD";
		case B_CPU_VENDOR_CYRIX:	return "Cyrix";
		case B_CPU_VENDOR_IDT:		return "IDT";
		case B_CPU_VENDOR_INTEL:	return "Intel";
		case B_CPU_VENDOR_NATIONAL_SEMICONDUCTOR:
									return "National Semiconductor";
		case B_CPU_VENDOR_RISE:		return "RISE";
		case B_CPU_VENDOR_TRANSMETA:	return "Transmeta";
		case B_CPU_VENDOR_VIA:		return "VIA";
		case B_CPU_VENDOR_IBM:		return "IBM";
		case B_CPU_VENDOR_MOTOROLA:	return "Motorola";
		case B_CPU_VENDOR_NEC:		return "NEC";
		case B_CPU_VENDOR_HYGON:	return "Hygon";
		case B_CPU_VENDOR_SUN:		return "Sun";
		case B_CPU_VENDOR_FUJITSU:	return "Fujitsu";
		default:					return NULL;
	}
}

#ifdef AP_HAVE_CPUID
// Reads the 48 byte processor brand string (CPUID 0x80000002 - 0x80000004),
// for example "Intel(R) Core(TM) i7-12700K CPU @ 2.10GHz" or
// "AMD Ryzen 9 5900X 12-Core Processor".  Returns false when the processor
// reports none.
static bool
cpuBrandString(char* out, size_t outSize)
{
	if(__get_cpuid_max(0x80000000u, NULL) < 0x80000004u)
		return false;

	char	brand[49];
	for(unsigned leaf = 0x80000002u; leaf <= 0x80000004u; leaf++)
	{
		unsigned	regs[4] = { 0, 0, 0, 0 };
		__cpuid(leaf, regs[0], regs[1], regs[2], regs[3]);
		memcpy(brand + (leaf - 0x80000002u) * 16, regs, sizeof(regs));
	}
	brand[48] = '\0';

	// vendors blank pad the string on both ends
	char* first = brand;
	while(*first == ' ')
		first++;
	char* last = first + strlen(first);
	while(last > first && last[-1] == ' ')
		last--;
	*last = '\0';

	if(*first == '\0')
		return false;

	snprintf(out, outSize, "%s", first);
	return true;
}

// Drops the vendor's decoration so the model name fits on the panel:
// "Intel(R) Core(TM) i7-12700K CPU @ 2.10GHz" -> "Intel Core i7-12700K"
// "AMD Ryzen 9 5900X 12-Core Processor"       -> "AMD Ryzen 9 5900X"
static void
cpuModelName(const char* brand, char* out, size_t outSize)
{
	const char*	p = brand;
	size_t		used = 0;

	out[0] = '\0';
	while(*p != '\0')
	{
		while(*p == ' ' || *p == '\t')
			p++;

		const char* word = p;
		while(*p != '\0' && *p != ' ' && *p != '\t')
			p++;

		size_t length = (size_t)(p - word);
		if(length == 0)
			break;
		// the clock after '@' is already shown by the meter, "with ..."
		// only appends the GPU of the processor
		if(word[0] == '@')
			break;
		if(length == 4 && memcmp(word, "with", 4) == 0)
			break;

		bool decoration = (length == 3 && memcmp(word, "(R)", 3) == 0)
			|| (length == 3 && memcmp(word, "(r)", 3) == 0)
			|| (length == 4 && memcmp(word, "(TM)", 4) == 0)
			|| (length == 4 && memcmp(word, "(tm)", 4) == 0)
			|| (length == 3 && memcmp(word, "CPU", 3) == 0)
			|| (length == 9 && memcmp(word, "Processor", 9) == 0)
			|| (length > 5 && memcmp(word + length - 5, "-Core", 5) == 0);
		if(decoration)
			continue;

		if(used + length + 2 > outSize)
			break;
		if(used != 0)
			out[used++] = ' ';
		memcpy(out + used, word, length);
		used += length;
		out[used] = '\0';
	}
}
#endif // AP_HAVE_CPUID

// Fills in a human readable CPU name and the nominal clock speed in Hz.
// Either value may end up empty/zero when the system does not report it.
static void
getCPUIdentification(char* name, size_t nameSize, uint64* frequency)
{
	enum cpu_platform		platform = B_CPU_UNKNOWN;
	enum cpu_vendor			vendor = B_CPU_VENDOR_UNKNOWN;
	uint64					defaultFrequency = 0;
	uint32					nodeCount = 0;

	// Same two call pattern the BeOS compatibility layer uses: first ask
	// for the number of topology nodes, then fill the array.
	if(get_cpu_topology_info(NULL, &nodeCount) == B_OK && nodeCount > 0)
	{
		cpu_topology_node_info* nodes = new cpu_topology_node_info[nodeCount];
		if(nodes != NULL && get_cpu_topology_info(nodes, &nodeCount) == B_OK)
		{
			for(uint32 i = 0; i < nodeCount; i++)
			{
				switch(nodes[i].type)
				{
					case B_TOPOLOGY_ROOT:
						platform = nodes[i].data.root.platform;
						break;
					case B_TOPOLOGY_PACKAGE:
						vendor = nodes[i].data.package.vendor;
						break;
					case B_TOPOLOGY_CORE:
						if(defaultFrequency == 0)
							defaultFrequency = nodes[i].data.core.default_frequency;
						break;
					default:
						break;
				}
			}
		}
		delete[] nodes;
	}

	const char* cpuName = cpuVendorName(vendor);
	if(cpuName == NULL)
		cpuName = cpuPlatformName(platform);

	name[0] = '\0';
#ifdef AP_HAVE_CPUID
	// recent Intel and AMD processors are recognised by their model name
	char brand[64];
	if(cpuBrandString(brand, sizeof(brand)))
		cpuModelName(brand, name, nameSize);
#endif
	if(name[0] == '\0')
		snprintf(name, nameSize, "%s", cpuName);

	*frequency = defaultFrequency;
}

AnalogView::AnalogView(BRect r, int cpu) : GaugeView(r, cpu)
{
	oldActiveTime = cpuActiveTime(cpu);
	oldSystemTime = system_time();

	InitObject(r, cpu, true);
}

AnalogView::AnalogView(BMessage *data) : GaugeView(data)
{
	int cpu = data->FindInt32("cpu");

	oldActiveTime = cpuActiveTime(cpu);
	oldSystemTime = system_time();

	InitObject(data->FindRect("bounds"), cpu, false);
}

AnalogView*
AnalogView::Instantiate(BMessage *data)
{
	if (!validate_instantiation(data, "AnalogView"))
		return NULL;

	return new AnalogView(data);
}

short
AnalogView::SampleValue()
{
	double	now = system_time();
	double	activeTime = cpuActiveTime(Id());
	double	elapsed = now - oldSystemTime;

	short	value = 0;
	if (elapsed > 0) {
		value = (short)(100 * (activeTime - oldActiveTime) / elapsed);
		if (value > 100)
			value = 100;
		if (value < 0)
			value = 0;
	}

	// store values for next iteration
	oldActiveTime = activeTime;
	oldSystemTime = now;

	return value;
}

const char*
AnalogView::ClassName() const
{
	return "AnalogView";
}

const char*
AnalogView::AddOnSignature() const
{
	return kSignature;
}

const char*
AnalogView::AppName() const
{
	return "AnalogPulse Lite";
}

void
AnalogView::Title(char *buf, size_t size)
{
	snprintf(buf, size, "CPU [%d]", Id());
}

void
AnalogView::Identification(char *name, size_t nameSize, char *extra,
	size_t extraSize)
{
	uint64	cpu_speed = 0;

	getCPUIdentification(name, nameSize, &cpu_speed);

	extra[0] = '\0';
	if (cpu_speed > 0) {
		// round the reported clock to a tidy figure
		int	target = (int)(cpu_speed / 1000000);
		int	frac = target % 100;
		int	delta = -frac;
		int	freqs[] = { 100, 50, 25, 75, 33, 67, 20, 40, 60, 80, 10, 30, 70,
			90 };

		for (size_t x = 0; x < sizeof(freqs) / sizeof(freqs[0]); x++) {
			int ndelta = freqs[x] - frac;
			if (abs(ndelta) < abs(delta))
				delta = ndelta;
		}
		snprintf(extra, extraSize, "%dMHz", target + delta);
	}
}
