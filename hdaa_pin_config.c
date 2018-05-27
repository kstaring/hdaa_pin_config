#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>


static const char *HDA_COLORS[16] = {"Unknown", "Black", "Grey", "Blue",
    "Green", "Red", "Orange", "Yellow", "Purple", "Pink", "Res.A", "Res.B",
    "Res.C", "Res.D", "White", "Other"};

static const char *HDA_DEVS[16] = {"Line-out", "Speaker", "Headphones", "CD",
    "SPDIF-out", "Digital-out", "Modem-line", "Modem-handset", "Line-in",
    "AUX", "Mic", "Telephony", "SPDIF-in", "Digital-in", "Res.E", "Other"};

static const char *HDA_CONNS[4] = {"Jack", "None", "Fixed", "Both"};

static const char *HDA_CONNECTORS[16] = {
    "Unknown", "1/8", "1/4", "ATAPI", "RCA", "Optical", "Digital", "Analog",
    "DIN", "XLR", "RJ-11", "Combo", "0xc", "0xd", "0xe", "Other" };

static const char *HDA_LOCS[64] = {
    "0x00", "Rear", "Front", "Left", "Right", "Top", "Bottom", "Rear-panel",
        "Drive-bay", "0x09", "0x0a", "0x0b", "0x0c", "0x0d", "0x0e", "0x0f",
    "Internal", "0x11", "0x12", "0x13", "0x14", "0x15", "0x16", "Riser",
        "0x18", "Onboard", "0x1a", "0x1b", "0x1c", "0x1d", "0x1e", "0x1f",
    "External", "Ext-Rear", "Ext-Front", "Ext-Left", "Ext-Right", "Ext-Top", "Ext-Bottom", "0x07",
        "0x28", "0x29", "0x2a", "0x2b", "0x2c", "0x2d", "0x2e", "0x2f",
    "Other", "0x31", "0x32", "0x33", "0x34", "0x35", "Other-Bott", "Lid-In",
        "Lid-Out", "0x39", "0x3a", "0x3b", "0x3c", "0x3d", "0x3e", "0x3f" };

static const char *HDA_GPIO_ACTIONS[8] = {
    "keep", "set", "clear", "disable", "input", "0x05", "0x06", "0x07"};

static const char *HDA_HDMI_CODING_TYPES[18] = {
    "undefined", "LPCM", "AC-3", "MPEG1", "MP3", "MPEG2", "AAC-LC", "DTS",
    "ATRAC", "DSD", "E-AC-3", "DTS-HD", "MLP", "DST", "WMAPro", "HE-AAC",
    "HE-AACv2", "MPEG-Surround"
};


#define HDA_CONFIG_DEFAULTCONF_SEQUENCE_MASK          0x0000000f
#define HDA_CONFIG_DEFAULTCONF_SEQUENCE_SHIFT         0
#define HDA_CONFIG_DEFAULTCONF_ASSOCIATION_MASK               0x000000f0
#define HDA_CONFIG_DEFAULTCONF_ASSOCIATION_SHIFT      4
#define HDA_CONFIG_DEFAULTCONF_MISC_MASK              0x00000f00
#define HDA_CONFIG_DEFAULTCONF_MISC_SHIFT             8
#define HDA_CONFIG_DEFAULTCONF_COLOR_MASK             0x0000f000
#define HDA_CONFIG_DEFAULTCONF_COLOR_SHIFT            12
#define HDA_CONFIG_DEFAULTCONF_CONNECTION_TYPE_MASK   0x000f0000
#define HDA_CONFIG_DEFAULTCONF_CONNECTION_TYPE_SHIFT  16
#define HDA_CONFIG_DEFAULTCONF_DEVICE_MASK            0x00f00000
#define HDA_CONFIG_DEFAULTCONF_DEVICE_SHIFT           20
#define HDA_CONFIG_DEFAULTCONF_LOCATION_MASK          0x3f000000
#define HDA_CONFIG_DEFAULTCONF_LOCATION_SHIFT         24
#define HDA_CONFIG_DEFAULTCONF_CONNECTIVITY_MASK      0xc0000000
#define HDA_CONFIG_DEFAULTCONF_CONNECTIVITY_SHIFT     30

uint32_t
hdaa_pin_string_to_hex(uint32_t config, const char *str)
{
	char buf[256];
	char *key, *value, *rest, *bad;
	int ival, i;

	strlcpy(buf, str, sizeof(buf));
	rest = buf;
	while ((key = strsep(&rest, "=")) != NULL) {
		value = strsep(&rest, " \t");
		if (value == NULL)
			break;
		ival = strtol(value, &bad, 10);
		if (strcmp(key, "seq") == 0) {
			config &= ~HDA_CONFIG_DEFAULTCONF_SEQUENCE_MASK;
			config |= ((ival << HDA_CONFIG_DEFAULTCONF_SEQUENCE_SHIFT) &
			    HDA_CONFIG_DEFAULTCONF_SEQUENCE_MASK);
		} else if (strcmp(key, "as") == 0) {
			config &= ~HDA_CONFIG_DEFAULTCONF_ASSOCIATION_MASK;
			config |= ((ival << HDA_CONFIG_DEFAULTCONF_ASSOCIATION_SHIFT) &
			    HDA_CONFIG_DEFAULTCONF_ASSOCIATION_MASK);
		} else if (strcmp(key, "misc") == 0) {
			config &= ~HDA_CONFIG_DEFAULTCONF_MISC_MASK;
			config |= ((ival << HDA_CONFIG_DEFAULTCONF_MISC_SHIFT) &
			    HDA_CONFIG_DEFAULTCONF_MISC_MASK);
		} else if (strcmp(key, "color") == 0) {
			config &= ~HDA_CONFIG_DEFAULTCONF_COLOR_MASK;
			if (bad[0] == 0) {
				config |= ((ival << HDA_CONFIG_DEFAULTCONF_COLOR_SHIFT) &
				    HDA_CONFIG_DEFAULTCONF_COLOR_MASK);
			}
			for (i = 0; i < 16; i++) {
				if (strcasecmp(HDA_COLORS[i], value) == 0) {
					config |= (i << HDA_CONFIG_DEFAULTCONF_COLOR_SHIFT);
					break;
				}
			}
		} else if (strcmp(key, "ctype") == 0) {
			config &= ~HDA_CONFIG_DEFAULTCONF_CONNECTION_TYPE_MASK;
			if (bad[0] == 0) {
			config |= ((ival << HDA_CONFIG_DEFAULTCONF_CONNECTION_TYPE_SHIFT) &
			    HDA_CONFIG_DEFAULTCONF_CONNECTION_TYPE_MASK);
			}
			for (i = 0; i < 16; i++) {
				if (strcasecmp(HDA_CONNECTORS[i], value) == 0) {
					config |= (i << HDA_CONFIG_DEFAULTCONF_CONNECTION_TYPE_SHIFT);
					break;
				}
			}
		} else if (strcmp(key, "device") == 0) {
			config &= ~HDA_CONFIG_DEFAULTCONF_DEVICE_MASK;
			if (bad[0] == 0) {
				config |= ((ival << HDA_CONFIG_DEFAULTCONF_DEVICE_SHIFT) &
				    HDA_CONFIG_DEFAULTCONF_DEVICE_MASK);
				continue;
			}
			for (i = 0; i < 16; i++) {
				if (strcasecmp(HDA_DEVS[i], value) == 0) {
					config |= (i << HDA_CONFIG_DEFAULTCONF_DEVICE_SHIFT);
					break;
				}
			}
		} else if (strcmp(key, "loc") == 0) {
			config &= ~HDA_CONFIG_DEFAULTCONF_LOCATION_MASK;
			if (bad[0] == 0) {
				config |= ((ival << HDA_CONFIG_DEFAULTCONF_LOCATION_SHIFT) &
				    HDA_CONFIG_DEFAULTCONF_LOCATION_MASK);
				continue;
			}
			for (i = 0; i < 64; i++) {
				if (strcasecmp(HDA_LOCS[i], value) == 0) {
					config |= (i << HDA_CONFIG_DEFAULTCONF_LOCATION_SHIFT);
					break;
				}
			}
		} else if (strcmp(key, "conn") == 0) {
			config &= ~HDA_CONFIG_DEFAULTCONF_CONNECTIVITY_MASK;
			if (bad[0] == 0) {
				config |= ((ival << HDA_CONFIG_DEFAULTCONF_CONNECTIVITY_SHIFT) &
				    HDA_CONFIG_DEFAULTCONF_CONNECTIVITY_MASK);
				continue;
			}
			for (i = 0; i < 4; i++) {
				if (strcasecmp(HDA_CONNS[i], value) == 0) {
					config |= (i << HDA_CONFIG_DEFAULTCONF_CONNECTIVITY_SHIFT);
					break;
				}
			}
		}
	}
	return (config);
}

void
hdaa_pin_hex_to_string(const uint32_t config, char *str, int buflen)
{
	int idx = 0;
	char *buf, *endptr, *val;
	uint32_t tmp, part;

	buf = str;
	endptr = str + buflen;

	// SEQUENCE
	if ((tmp = config & HDA_CONFIG_DEFAULTCONF_SEQUENCE_MASK)) {
		part = tmp >> HDA_CONFIG_DEFAULTCONF_SEQUENCE_SHIFT;
		buf += snprintf(buf, endptr - buf, "seq=%d ", part);
	}

	// ASSOCIATION
	if ((tmp = config & HDA_CONFIG_DEFAULTCONF_ASSOCIATION_MASK)) {
		part = tmp >> HDA_CONFIG_DEFAULTCONF_ASSOCIATION_SHIFT;
		buf += snprintf(buf, endptr - buf, "as=%d ", part);
	}

	// MISC
	if ((tmp = config & HDA_CONFIG_DEFAULTCONF_MISC_MASK)) {
		part = tmp >> HDA_CONFIG_DEFAULTCONF_MISC_SHIFT;
		buf += snprintf(buf, endptr - buf, "misc=%d ", part);
	}

	// COLOR
	if ((tmp = config & HDA_CONFIG_DEFAULTCONF_COLOR_MASK)) {
		part = tmp >> HDA_CONFIG_DEFAULTCONF_COLOR_SHIFT;
		val = part < 16 ? HDA_COLORS[part] : "INVALID";
		buf += snprintf(buf, endptr - buf, "color=%s ", val);
	}

	// CTYPE
	if ((tmp = config & HDA_CONFIG_DEFAULTCONF_CONNECTION_TYPE_MASK)) {
		part = tmp >> HDA_CONFIG_DEFAULTCONF_CONNECTION_TYPE_SHIFT;
		val = part < 16 ? HDA_CONNECTORS[part] : "INVALID";
		buf += snprintf(buf, endptr - buf, "ctype=%s ", val);
	}

	// DEVICE
	if ((tmp = config & HDA_CONFIG_DEFAULTCONF_DEVICE_MASK)) {
		part = tmp >> HDA_CONFIG_DEFAULTCONF_DEVICE_SHIFT;
		val = part < 16 ? HDA_DEVS[part] : "INVALID";
		buf += snprintf(buf, endptr - buf, "device=%s ", val);
	}

	// LOCATION
	if ((tmp = config & HDA_CONFIG_DEFAULTCONF_LOCATION_MASK)) {
		part = tmp >> HDA_CONFIG_DEFAULTCONF_LOCATION_SHIFT;
		val = part < 64 ? HDA_LOCS[part] : "INVALID";
		buf += snprintf(buf, endptr - buf, "loc=%s ", val);
	}

	// CONNECTIVITY
	if ((tmp = config & HDA_CONFIG_DEFAULTCONF_CONNECTIVITY_MASK)) {
		part = tmp >> HDA_CONFIG_DEFAULTCONF_CONNECTIVITY_SHIFT;
		val = part < 4 ? HDA_CONNS[part] : "INVALID";
		buf += snprintf(buf, endptr - buf, "conn=%s ", val);
	}

	if (buf > str && buf[-1] == ' ')
		buf[-1] = '\0';
}

#define MAXBUF 1024

int main(int argc, char **argv)
{
	char buf[MAXBUF];
	const char *arg;
	uint32_t config = 0;

	if (argc != 2) {
		printf("Usage: %s <pin hex (must start with 0x)|pin string>\n\n", argv[0]);
		return (EINVAL);
	}

	arg = argv[1];

	if (strncmp(arg, "0x", 2) == 0) {
		config = strtol(arg + 2, NULL, 16);
		bzero(buf, 1024);
		hdaa_pin_hex_to_string(config, buf, MAXBUF);
		printf("0x%08x = \"%s\"\n", config, buf);
 	} else {
		config = hdaa_pin_string_to_hex(config, arg);
		printf("\"%s\" = 0x%08x\n", arg, config);
	}

	return (0);
}
