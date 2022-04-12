#include <stdio.h>
#include <wchar.h>
#include <string.h>
#include <stdlib.h>
#include "hidapi.h"
#include <stdint.h>
#include <ctype.h>

/* The factory programming commands are not exposed in the 
 * public code drop of htt_util. */
#ifdef HTT_UTIL_WITH_FACTORY_COMMANDS
#  include "htt_util_factory.h"
#endif

size_t g_currentDevice = 0;
size_t g_device_count = 0;
int g_verbose = 0;

hid_device **g_handles = NULL;

// Headers needed for sleeping.
#ifdef _WIN32
	#include <windows.h>
#else
	#include <unistd.h>
	#define min(a,b) (((a)<(b))?(a):(b))
#endif

#define TOUCH_NONE		0
#define TOUCH_RESISTIVE 1
#define TOUCH_MXTxx     2       
#define TOUCH_GT9xx     3       
#define TOUCH_FT5xx     4
#define TOUCH_ILI25xx   5

#define REPORT_DRIVER_TYPE     4
#define REPORT_CALMATRIX       6 
#define REPORT_MXT_SENSITIVITY 7
#define REPORT_SCREENROTATION  8
#define REPORT_FWREV           9
#define REPORT_BACKLIGHT       10
#define REPORT_HAPTIC		   11
#define REPORT_PIEZO		   12
#define REPORT_MODULEID        13
#define REPORT_TOUCHFEEDBACK   15	
#define REPORT_TOUCHDIM		   16	
#define REPORT_PCAPCALIBRATE   17	
#define REPORT_BACKLIGHT_FADE  18
#define REPORT_FACTORY_RESET   19
#define REPORT_ALARM           20

const char* TouchTypes[] = { "None", "Resistive", "MXTxx", "GT9xx", "FT5xx", "ILI25xx" };
const char* Rotation[] = { "0", "90", "180", "270"};
const char* Sensitivity[] = { "normal", "high", "extra" };
const char* TouchFeedbackTypes[] = { "None" ,"Haptic", "Piezo", "Haptic and Piezo" ,"Invalid" };

typedef void(*parm_handler)(hid_device* device, char* argv[], int start_index);
typedef struct 
{
	const char* name;
	const size_t parameter_count;
	parm_handler handler;
} cli_parm;

int checkhtt(hid_device *handle)
{
	if (!handle)
	{
		printf("No HTT detected\n");
		exit(0);
		return 0;
	}
	return 1;
}

int get_driver(hid_device *handle)
{
	unsigned char buf[256];
	buf[0] = REPORT_DRIVER_TYPE;
	int res = hid_get_feature_report(handle, buf, 2);
	if (res < 0) {
		return 0;
	}
	return buf[1];
}

int get_fwrev(hid_device *handle)
{
	unsigned char buf[256];
	buf[0] = REPORT_FWREV;
	int res = hid_get_feature_report(handle, buf, 5);
	if (res < 0) {
		return 0;
	}
	int*rev = (int*)&buf[1];
	return *rev;
}

int get_rotation(hid_device *handle)
{
	unsigned char buf[256];
	buf[0] = REPORT_SCREENROTATION;
	int res = hid_get_feature_report(handle, buf, 2);
	if (res < 0) {
		return -1;
	}
	return buf[1];
}

int set_rotation(hid_device *handle, int rotation)
{
	unsigned char buf[256];
	buf[0] = REPORT_SCREENROTATION;
	buf[1] = rotation;
	int res = hid_send_feature_report(handle, buf, 2);
	if (res < 0) {
		return 0;
	}
	return 1;
}

int get_calmatrix(hid_device *handle, unsigned char*out_buffer, size_t buffersize)
{
	unsigned char buf[256];
	if (buffersize < 56)
		return -1;

	buf[0] = REPORT_CALMATRIX;
	int res = hid_get_feature_report(handle, buf, 57);
	if (res < 0) {
		return -1;
	}
	int len = min(res - 1, 56);
	memcpy(out_buffer, &buf[1], len);
	return len;
}

int set_calmatrix(hid_device *handle, unsigned char*matrix, size_t buffersize)
{
	unsigned char buf[256];

	if (buffersize != 56) /* Matrix needs to be 56 bytes*/
		return 0;

	buf[0] = REPORT_CALMATRIX;
	memcpy(&buf[1], matrix, 56);
	int res = hid_send_feature_report(handle, buf, 57);
	if (res < 0) {
		return 0;
	}
	return 1;
}


int set_sensitivity(hid_device *handle, int sensitivity)
{
	unsigned char buf[256];
	buf[0] = REPORT_MXT_SENSITIVITY;
	buf[1] = sensitivity;
	int res = hid_send_feature_report(handle, buf, 2);
	if (res < 0) {
		return 0;
	}
	return 1;
}

int get_sensitivity(hid_device *handle)
{
	unsigned char buf[256];
	buf[0] = REPORT_MXT_SENSITIVITY;
	int res = hid_get_feature_report(handle, buf, 2);
	if (res < 0) {
		return 0;
	}
	return buf[1];
}

int get_backlight(hid_device *handle)
{
	unsigned char buf[256];
	buf[0] = REPORT_BACKLIGHT;
	int res = hid_get_feature_report(handle, buf, 3);
	if (res < 0) {
		return -1;
	}
	return buf[1];
}

int get_backlight_fade(hid_device *handle)
{
	unsigned char buf[256];
	buf[0] = REPORT_BACKLIGHT_FADE;
	int res = hid_get_feature_report(handle, buf, 4);
	if (res < 0) {
		return -1;
	}
	return buf[2] << 8 | buf[1];
}

int set_backlight(hid_device *handle, uint8_t backlight, uint8_t save)
{
	unsigned char buf[256];
	buf[0] = REPORT_BACKLIGHT;
	buf[1] = backlight;
	buf[2] = save ? 1 : 0;
	int res = hid_send_feature_report(handle, buf, 3);
	if (res < 0) {
		return 0;
	}
	return 1;
}

int set_fade(hid_device *handle, uint16_t fade_time, uint8_t save)
{
	unsigned char buf[256];
	buf[0] = REPORT_BACKLIGHT_FADE;
	buf[1] = (fade_time & 0xff00) >> 8;
	buf[2] = (fade_time & 0xff);
	buf[3] = save ? 1 : 0;
	int res = hid_send_feature_report(handle, buf, 4);
	if (res < 0) {
		return 0;
	}
	return 1;
}

int get_moduleID(hid_device *handle)
{
	unsigned char buf[256];
	buf[0] = REPORT_MODULEID;
	int res = hid_get_feature_report(handle, buf, 3);
	if (res < 0) {
		return -1;
	}
	return buf[1];
}



void rotate_touch(hid_device* device, char* argv[], int start_index)
{
	if (checkhtt(device))
	{
		for (int i = 0; i < 4; i++)
		{
			if (strcmp(argv[start_index + 1], Rotation[i]) == 0)
			{
				int success = set_rotation(device, i);
				printf("Setting rotation to %s : %s\n", Rotation[i], success ? "Success!" : "Failed.");
				return;
			}
		}
		printf("Invalid parameter for rotation : %s\n", argv[start_index + 1]);
	}
}

void sensitivty(hid_device* device, char* argv[], int start_index)
{
	if (checkhtt(device))
	{
		bool validDevice = false;
		validDevice |= get_driver(device) == TOUCH_MXTxx;
		validDevice |= get_driver(device) == TOUCH_GT9xx;
		if (!validDevice)
		{
			printf("Setting sensitivty not supported on %s driver", TouchTypes[get_driver(device)]);
			return;
		}
		for (int i = 0; i < 3; i++)
		{
			if (strcmp(argv[start_index + 1], Sensitivity[i]) == 0)
			{
				int success = set_sensitivity(device, i);
				printf("Setting sensitivity to %s : %s\n", Sensitivity[i], success ? "Success!\n" : "Failed.\n");
				if (success)
				{
					printf("The sensitivity command reboots the unit, further commands will not executed.\n");
					exit(0);
				}
				return;
			}
		}
		printf("Invalid parameter for sensitivity : %s\n", argv[start_index + 1]);
	}
}

void savecalibration(hid_device* device, char* argv[], int start_index)
{
	if (checkhtt(device))
	{
		if (get_driver(device) != TOUCH_RESISTIVE)
		{
			printf("Saving calibration matrix is not supported on %s driver", TouchTypes[get_driver(device)]);
			return;
		}
		unsigned char buffer[80];
		if (get_calmatrix(device, buffer, sizeof(buffer)) == 56) 
		{
			FILE* f = fopen(argv[start_index + 1], "wb");
			if (!f)
			{
				printf("error opening %s\n", argv[start_index + 1]);
				return;
			}
			fwrite(buffer, 1, 56, f);
			fclose(f);
			printf("Calibration matrix written to %s\n", argv[start_index + 1]);
		}
		else
		{
			printf("Error retrieving calibration matrix.");
		}
	}
}

void loadcalibration(hid_device* device, char* argv[], int start_index)
{
	if (checkhtt(device))
	{
		if (get_driver(device) != TOUCH_RESISTIVE)
		{
			printf("Saving calibration matrix is not supported on %s driver", TouchTypes[get_driver(device)]);
			return;
		}
		unsigned char buffer[80];
		FILE* f = fopen(argv[start_index + 1], "rb");
		if (!f)
		{
			printf("error opening %s\n", argv[start_index + 1]);
			return;
		}

		fseek(f, 0, SEEK_END);    
		int file_size = ftell(f); 
		if (file_size != 56)
		{
			fclose(f);
			printf("File size mismatch, %s is %d bytes, expected 56.\n", argv[start_index + 1], file_size);
		}
		fseek(f, 0, SEEK_SET); // seek back to beginning of file
		int size = fread(buffer, 1, 56, f);
		fclose(f);
		if (set_calmatrix(device, buffer, size))
		{
			printf("Calibration matrix written to unit\nPlease reconnect the USB cable to load the new settings.\n");
		}
		else
		{
			printf("Error saving calibration matrix.");
		}
	}
}

int set_hapticduration(hid_device *handle, uint8_t duration)
{
	unsigned char buf[256];
	buf[0] = REPORT_HAPTIC;
	buf[1] = duration;
	int res = hid_send_feature_report(handle, buf, 2);
	if (res < 0) {
		return 0;
	}
	return 1;
}

int set_piezoduration(hid_device *handle, uint8_t duration)
{
	unsigned char buf[256];
	buf[0] = REPORT_PIEZO;
	buf[1] = duration;
	int res = hid_send_feature_report(handle, buf, 2);
	if (res < 0) {
		return 0;
	}
	return 1;
}

int set_touchfeedback(hid_device *handle, uint8_t setting)
{
	unsigned char buf[256];
	buf[0] = REPORT_TOUCHFEEDBACK;
	buf[1] = setting;
	int res = hid_send_feature_report(handle, buf, 2);
	if (res < 0) {
		return 0;
	}
	return 1;
}

int get_touchfeedback(hid_device *handle)
{
	unsigned char buf[256];
	buf[0] = REPORT_TOUCHFEEDBACK;
	int res = hid_get_feature_report(handle, buf, 2);
	if (res < 0) {
		return 0;
	}
	return buf[1];
}

int set_touchdim(hid_device *handle, int brightness[4], int timeout[4])
{
	unsigned char buf[256];
	buf[0] = REPORT_TOUCHDIM;
	for (int i = 0; i < 4; i++)
	{
		buf[i + 1] = brightness[i] & 0xFF;
		buf[(i * 2) + 5]= ((timeout[i] & 0xFF00) >> 8);
		buf[(i * 2) + 6] = (timeout[i] & 0xFF);
	}
	int res = hid_send_feature_report(handle, buf, 13);
	if (res < 0) {
		return 0;
	}
	return 1;
}

int factory_reset(hid_device *handle)
{
	unsigned char buf[256];
	buf[0] = REPORT_FACTORY_RESET;
	buf[1] = 0;
	int res = hid_send_feature_report(handle, buf, 2);
	if (res < 0) {
		return 0;
	}
	return 1;
}

int do_alarm(hid_device *handle, uint8_t alarm_type, uint16_t duration, uint8_t blink)
{
	unsigned char buf[256];
	buf[0] = REPORT_ALARM;
	buf[1] = alarm_type;
	buf[2] = ((duration & 0xFF00) >> 8);
	buf[3] = (duration & 0xFF);
	buf[4] = blink;
	int res = hid_send_feature_report(handle, buf, 5);
	if (res < 0) {
		printf("Alarm fail\n");
		return 0;
	}
	printf("Alarm success\n");
	return 1;
}

int get_touchdim(hid_device *handle, int brightness[4], int timeout[4])
{
	unsigned char buf[256] = { 0 };
	buf[0] = REPORT_TOUCHDIM;
	int res = hid_get_feature_report(handle, buf, 13);
	if (res < 0) {
		return 0;
	}
	for (int i = 0; i < 4; i++)
	{
		brightness[i] = buf[1 + i];
		timeout[i] = buf[(i * 2) + 5] << 8 | buf[(i * 2) + 6];
	}

	return 1;
}

int capcalibrate(hid_device* handle)
{
	unsigned char buf[256];
	buf[0] = REPORT_PCAPCALIBRATE;	
	buf[1] = 0;
	int res = hid_send_feature_report(handle, buf, 2);
	if (res < 0) {
		return 0;
	}
	return 1;
}

void help(hid_device* device, char* argv[], int start_index)
{
	printf("Usage: htt_util [options]\n\n");
	printf("options:\n");
	printf(" --help\n");
	printf("    Show help (this message).\n\n");
	printf(" --device [id]\n");
	printf("    Selects the target for the following commands in setups where multiple HTTs\n");
	printf("    are connected. (0 = default)\n\n");
	printf(" --loadcalibration [filename]\n");
	printf("    Load the calibration data from a file, only available on resistive\n");
	printf("    touch screens.\n\n");
	printf(" --rotatetouch [degrees]\n");
	printf("    Sets and saves the rotation for the touch panel (visual output will not\n");
	printf("    change orientation.) Normally the host OS should take care of screen\n");
	printf("    rotation, if the host OS does not support this, this options offers\n");
	printf("    the option to apply the rotation on the device\n");
	printf("    degrees can be [0, 90, 180, 270]. \n");
	printf("    \n");
	printf(" --savecalibration [filename]\n");
	printf("    Save the calibration data to a file, only available on resistive touch\n");
	printf("    screens.\n\n");
	printf(" --scan\n");
	printf("    Scan for HTT modules and display their settings.\n\n");
	printf(" --sensitivity [level]\n");
	printf("    Sets the sensitivity of the touch panel.\n");
	printf("    This setting is only available on mxt and 7\" gt9xx driver based modules.\n");
	printf("    Attempting set this option to anything besides 'normal' on any other product\n");
	printf("    will lead to undefined behavior and is not recommended.\n\n");
	printf("    level for mxt can be [normal,high,extra]\n");
	printf("    level for 7\" GT9xx can be [normal,high]\n\n\n");
	printf("===Following commands are for PCB Rev 1.5 or higher only====\n\n"); 
	printf(" --backlight [setting]\n");
	printf("    set backlight brightness [0-255]\n\n");
	printf(" --backlightfade [time in ms]\n");
	printf("    set and save the response time to a backlight brightness change\n\n");
	printf(" --backlightset [setting]\n");
	printf("    set and save backlight brightness [0-255]\n\n");
	printf(" --haptic [duration]\n");
	printf("	enable haptic feedback for [duration]\n");
	printf("    [duration] duration for haptic feedback (in 100ms increments)\n");
	printf("    for 1 second - use 10 \n\n");
	printf(" --piezo [duration]\n");
	printf("	sound the piezo at 440hz for [duration]\n");
	printf("    [duration] duration for piezo (in 100ms increments)\n");
	printf("    for 1 second - use 10\n\n");
	printf(" --alarm [type] [duration] [flash]\n");
	printf("    The alarm will continue until either one of the folowwing conditions occurs:\n");
	printf("	     - The duration of the alarm is reached.\n");
	printf("	     - The screen is touched (touch capable models only).\n");
	printf("	     - The alarm is canceled by selecting alarm type 0.\n");
	printf("    type 0-17 alarm type [0 = off]\n");
	printf("    duration duration for the alarm (in 100ms increments, for 1 second - use 10), use -1 for no timeout, the alarm will continue until touch or cancelation.\n\n");
	printf("    flash - flashes per second, max = 10, off = 0\n\n");
	printf(" --touchfeedback\n");
	printf("    set touch feedback: [0 none, 1 haptic, 2 piezo, 3 haptic and piezo].\n\n");
	printf(" --touchdim [time1] [brightness1] [time2] [brightness2] [time3] [brightness3] [time4] [brightness4]\n");
	printf("    dim the display after [time] seconds of inactivity.\n");
	printf("    [time1]       [0-600] time in seconds since last touch\n");
	printf("    [brightness1] [0-255] brightness of the display 0 = Off, 255 is full brightness\n");
	printf("    [time2]       [0-600] time in seconds since [time1]\n");
	printf("    [brightness2] [0-255] brightness of the display 0 = Off, 255 is full brightness\n");
	printf("    [time3]       [0-600] time in seconds since [time3]\n");
	printf("    [brightness3] [0-255] brightness of the display 0 = Off, 255 is full brightness\n");
	printf("    [time4]       [0-600] time in seconds since [time4]\n");
	printf("    [brightness4] [0-255] brightness of the display 0 = Off, 255 is full brightness\n");
	printf("    to disable feature: --touchdim 0 0 0 0 0 0 0 0\n");
	printf("    Note: while time is specified in seconds, for convenience time can be postfixed with the letter 'm' for specify minutes ie 5m would automatically convert to 300 seconds.");
	printf(" --capcalibrate \n");
	printf("    PCAP calibrate\n\n");
	printf(" --factorydefaults\n");
	printf("    reset the unit to factory defaults\n");
}

void scan_internal(hid_device *handle, int index)
{
	if (checkhtt(handle))
	{
		int driver = get_driver(handle);
		int fwrev = get_fwrev(handle);
		printf("HTT Detected.\n");
		printf("- Device            : %d\n", index);
		printf("- Firmware Rev      : %d\n", fwrev);
		printf("- Driver Type       : %s (%d)\n", TouchTypes[driver], driver);
		printf("- Screen Rotation   : %s degrees\n", Rotation[get_rotation(handle)]);
		printf("- Default Backlight : %d \n", get_backlight(handle));
		int feedback = get_touchfeedback(handle);
		if (feedback > 3) {
			feedback = 4;
		}
		printf("- Touch feedback    : %d (%s) \n", feedback, TouchFeedbackTypes[feedback]);
		if (fwrev > 11762)
		{
			printf("- Backlight fade    : %d \n", get_backlight_fade(handle));
			int brightness[4] = { 0 };
			int timeout[4] = { 0 } ;
			if (get_touchdim(handle, brightness, timeout))
			{
				if (timeout[0]) {
					printf("- Backlight dimming \n");
					for (int i = 0; i < 4; i++)
					{
						if (!timeout[i])
							break;
						printf("\tAfter %d seconds set backlight to %d\n", timeout[i], brightness[i]);
					}
				}
				else {
					printf("- Backlight dimming : Disabled\n");
				}
			}
		}
		if (driver == TOUCH_MXTxx || driver == TOUCH_GT9xx)
		{
			int sens = get_sensitivity(handle);
			printf("- Touch Sensitivity : %d (%s).\n", sens, Sensitivity[sens]);
		}
		if (g_verbose && fwrev > 10656)
		{
			printf("- Module ID         : %d\n", get_moduleID(handle));
#ifdef HTT_UTIL_WITH_FACTORY_COMMANDS
			uint16_t customID = get_customID(handle);
			printf("- Custom ID         : %4x\n", customID);
#endif
		}
		printf("\n");
	}
}

void scan(hid_device *handle, char* argv[], int start_index)
{
	if(g_device_count) 
	{
		for (size_t i = 0; i < g_device_count; i++)
		{
			scan_internal(g_handles[i], i);
		}
	}
	else
	{
		printf("No HTT detected\n");
	}
}

void verbose(hid_device *handle, char* argv[], int start_index)
{
	g_verbose = 1;
}


void do_brightness(hid_device* device, char* argv[], int start_index, int save)
{
	if (checkhtt(device))
	{
		int brightness = atoi(argv[start_index + 1]);
		if (brightness > 255)
			brightness = 255;
		if (brightness < 0)
			brightness = 0;
		int success = set_backlight(device, brightness, save);
		printf("Setting brightness to %d : %s\n", brightness, success ? "Success!" : "Failed.");
		return;
	}
}

void do_fade(hid_device* device, char* argv[], int start_index, int save)
{
	if (checkhtt(device))
	{
		int fade = atoi(argv[start_index + 1]);
		if (fade > 0xffff)
			fade = 0xffff;
		if (fade < 0)
			fade = 0;
		int success = set_fade(device, fade,save);
		printf("Setting fade to %d : %s\n", fade, success ? "Success!" : "Failed.");
		return;
	}
}

void fadeset(hid_device* device, char* argv[], int start_index)
{
	do_fade(device, argv, start_index, 1);
}

void fade(hid_device* device, char* argv[], int start_index)
{
	do_fade(device, argv, start_index, 0);
}

void brightness(hid_device* device, char* argv[], int start_index)
{
	do_brightness(device, argv, start_index, 0);
}

void brightnessset(hid_device* device, char* argv[], int start_index)
{
	do_brightness(device, argv, start_index, 1);
}

void select_device(hid_device* device, char* argv[], int start_index)
{
	size_t devid = atoi(argv[start_index + 1]);
	if (devid < g_device_count)
	{
		g_currentDevice = devid;
		printf("Device %d selected.\n",devid);
	}
	else
	{
		printf("Invalid device ID : %d\n",devid);
	}
}

void hapticduration(hid_device* device, char* argv[], int start_index)
{
	if (checkhtt(device))
	{
		int duration = atoi(argv[start_index + 1]);
		if (duration > 100)
			duration = 100;
		if (duration < 0)
			duration = 0;
		int success = set_hapticduration(device, duration);
		printf("Setting haptic duration to %d : %s\n", duration, success ? "Success!" : "Failed.");
		return;
	}
}

void piezoduration(hid_device* device, char* argv[], int start_index)
{
	if (checkhtt(device))
	{
		int duration = atoi(argv[start_index + 1]);
		if (duration > 100)
			duration = 100;
		if (duration < 0)
			duration = 0;
		int success = set_piezoduration(device, duration);
		printf("Setting piezo duration to %d : %s\n", duration, success ? "Success!" : "Failed.");
		return;
	}
}

void touchfeedback(hid_device* device, char* argv[], int start_index)
{
	if (checkhtt(device))
	{
		int setting = atoi(argv[start_index + 1]);
		if (setting > 3)
			setting = 3;
		if (setting < 0)
			setting = 0;
		int success = set_touchfeedback(device, setting);
		printf("Setting touch feedback to %d : %s\n", setting, success ? "Success!" : "Failed.");
		return;
	}
}

bool parse_time(char* value, int* out_value)
{
	if (!value)
		return false;
	size_t len = strlen(value);
	
	if (len == 0)
		return false;
	
	*out_value = atoi(value);

	int last_is_number = isdigit(value[len - 1]);
	if (last_is_number)
		return true;
	/* if the value ends in m assume minutes. */
	if (value[len - 1] == 'm')
	{
		*out_value *= 60;
		return true;
	}
	/* if the value ends in s assume seconds, nothing needs to be done here
	 * but if 'm' is allowed people will try to use 's' */
	if (value[len - 1] == 's')
		return true;
	
	/* value does not end in a number, but does not end in 's' or 'm' something is wrong */
	return false;
}

void touchdim(hid_device* device, char* argv[], int start_index)
{
	if (checkhtt(device))
	{
		int time[4];
		int brightness[4];
		for (int i = 0; i < 4; i++)
		{
			if (!parse_time(argv[start_index + 1 + (i * 2)], &time[i]))
			{
				printf("Error parsing time value \"%s\"\n", argv[start_index + 1 + (i * 2)]);
				return;
			}
			brightness[i] = atoi(argv[start_index + 2 + (i * 2)]);
			if (brightness[i] > 255)
				brightness[i] = 255;
			if (brightness[i] < 0)
				brightness[i] = 0;
			if (time[i] < 0)
				time[i] = 0;
			if (time[i] > 0xffff)
				time[i] = 0xffff;
		}
		int success = set_touchdim(device, brightness, time);
		printf("Setting touch dimming properties : %s\n", success ? "Success!" : "Failed.");
		return;
	}
}

void pcapcalibrate(hid_device* device, char* argv[], int start_index)
{
	if (checkhtt(device))
	{
		int success = capcalibrate(device);
		printf("Calibrate: %s\n", success ? "Success!" : "Failed.");
		return;
	}
}

void alarm(hid_device* device, char* argv[], int start_index)
{
	if (checkhtt(device))
	{
		int type = atoi(argv[start_index + 1]);
		int duration = atoi(argv[start_index + 2]);
		int blink = atoi(argv[start_index + 3]);
		printf("alarm type = %d duration = %d blink=%d", type, duration,blink);
		do_alarm(device, type, duration,blink);
	}
}

void factorydefaults(hid_device* device, char* argv[], int start_index)
{
	if (checkhtt(device))
	{
		int success = factory_reset(device);
		printf("Factory Defaults: %s\n", success ? "Success!" : "Failed.");
		if (success)
		{
			printf("The Factory Defaults command reboots the unit, further commands are not executed.\n");
			exit(0);
		}
		return;
	}
}


cli_parm handlers[] =
{
	{ "--help", 1, 	help },
	{ "--scan",	1,	scan },
	{ "--verbose",  1, verbose },
	#ifdef HTT_UTIL_WITH_FACTORY_COMMANDS
	{ "--factory",  1, factory },
	{ "--customid", 2, customid },
	{ "--moduleid",	2, moduleid	},
	#endif
	{ "--rotatetouch", 2, rotate_touch },
	{ "--sensitivity", 2, sensitivty },
	{ "--savecalibration", 2, savecalibration },
	{ "--loadcalibration", 2, loadcalibration },
	{ "--backlight", 2,	brightness },
	{ "--backlightfade", 2,	fade },
	{ "--backlightset",	2, brightnessset },
	{ "--backlightfadeset",	2, fadeset},
	{ "--device", 2, select_device },
	{ "--haptic", 2, hapticduration },
	{ "--piezo", 2,	piezoduration},
	{ "--touchfeedback", 2, touchfeedback},
	{ "--touchdim", 9, touchdim},
	{ "--capcalibrate", 1, pcapcalibrate},
	{ "--factorydefaults", 1, factorydefaults},
	{ "--alarm", 4, alarm}
};

int main(int argc, char* argv[])
{
	if (hid_init())
	{
		printf("Error initializing USB\n");
		return -1;
	}

	hid_device_info* device, *iterator;
	device = iterator = hid_enumerate(0x1b3d, 0x14c9);
	/* count the number of devices available */
	while (iterator)
	{
		g_device_count++;
		iterator = iterator->next;
	}

	/* allocate ram for them */
	g_handles = (hid_device**)malloc(sizeof(void*)* g_device_count);
	
	/* store handles for all devices*/
	iterator = device;
	int curdevice = 0;
	while (iterator)
	{
		g_handles[curdevice++] = hid_open_path(iterator->path);
		iterator = iterator->next;
	}

	if (argc == 1)
	{
		help(NULL, NULL, 0);
	}
	else
	{
		for (size_t i = 1; i < (size_t)argc;)
		{
			bool work_done = false;
			size_t table_size = sizeof(handlers) / sizeof(cli_parm);
			for (size_t j = 0; j < table_size; j++)
			{
				if (strcmp(handlers[j].name, argv[i]) == 0)
				{
					if (i + handlers[j].parameter_count <= (size_t)argc)
					{
						hid_device *dev = g_currentDevice < g_device_count ? g_handles[g_currentDevice] : NULL;
						handlers[j].handler(dev, argv, i);
						i += handlers[j].parameter_count;
						work_done = true;
						break;
					}
					else
					{
						printf("missing parameter(s) for option : %s\n", argv[i]);
						break;
					}
				}
			}
			if (!work_done)
			{
				printf("Unknown parameter %s\n", argv[i]);
				break;
			}
		}
	}
	for (size_t i = 0; i < g_device_count; i++)
	{
		hid_close(g_handles[i]);
	}
	free(g_handles);
	hid_exit();
	return 0;
}
