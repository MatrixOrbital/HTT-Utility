#include <stdio.h>
#include <wchar.h>
#include <string.h>
#include <stdlib.h>
#include "hidapi.h"
#include <stdint.h>

int g_currentDevice = 0;
size_t g_device_count = 0;
int g_verbose = 0;

hid_device **g_handles = NULL;

// Headers needed for sleeping.
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

#define TOUCH_NONE 0
#define TOUCH_RESISTIVE 1
#define TOUCH_MXTxx 2
#define TOUCH_GT9xx 3
#define TOUCH_FT5xx 4

#define REPORT_DRIVER_TYPE 4
#define REPORT_CALMATRIX 6
#define REPORT_MXT_SENSITIVITY 7
#define REPORT_SCREENROTATION 8
#define REPORT_FWREV 9
#define REPORT_BACKLIGHT 10
#define REPORT_MOTOR 11
#define REPORT_PIEZO 12
#define REPORT_MODULEID 13
#define REPORT_CUSTOMID 14
#define REPORT_TOUCHFEEDBACK 15
#define REPORT_TOUCHDIM 16
#define REPORT_PCAPCALIBRATE 17

const char *TouchTypes[] = {"None", "Resistive", "MXTxx", "GT9xx", "FT5xx"};
const char *Rotation[] = {"0", "90", "180", "270"};
const char *Sensitivity[] = {"normal", "high", "extra"};

typedef void (*parm_handler)(hid_device *device, char *argv[], int start_index);
typedef struct {
  const char *name;
  const size_t parameter_count;
  parm_handler handler;
} cli_parm;

int checkhtt(hid_device *handle) {
  if (!handle) {
    printf("No HTT Touch screens detected\n");
    exit(0);
    return 0;
  }
  return 1;
}

int get_driver(hid_device *handle) {
  unsigned char buf[256];
  buf[0] = REPORT_DRIVER_TYPE;
  int res = hid_get_feature_report(handle, buf, 2);
  if (res < 0) {
    return 0;
    return 0;
  }
  return buf[1];
}

int get_fwrev(hid_device *handle) {
  unsigned char buf[256];
  buf[0] = REPORT_FWREV;
  int res = hid_get_feature_report(handle, buf, 5);
  if (res < 0) {
    return 0;
  }
  int *rev = (int *)&buf[1];
  return *rev;
}

int get_rotation(hid_device *handle) {
  unsigned char buf[256];
  buf[0] = REPORT_SCREENROTATION;
  int res = hid_get_feature_report(handle, buf, 2);
  if (res < 0) {
    return -1;
  }
  return buf[1];
}

int set_rotation(hid_device *handle, int rotation) {
  unsigned char buf[256];
  buf[0] = REPORT_SCREENROTATION;
  buf[1] = rotation;
  int res = hid_send_feature_report(handle, buf, 2);
  if (res < 0) {
    return 0;
  }
  return 1;
}

int get_calmatrix(hid_device *handle, unsigned char *out_buffer,
                  size_t buffersize) {
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

int set_calmatrix(hid_device *handle, unsigned char *matrix,
                  size_t buffersize) {
  unsigned char buf[256];

  if (buffersize != 56) /* Matrix needs to be 56 bytes*/
    return -1;

  buf[0] = REPORT_CALMATRIX;
  memcpy(&buf[1], matrix, 56);
  int res = hid_send_feature_report(handle, buf, 57);
  if (res < 0) {
    return 0;
  }
  return 1;
}

int set_sensitivity(hid_device *handle, int sensitivity) {
  unsigned char buf[256];
  buf[0] = REPORT_MXT_SENSITIVITY;
  buf[1] = sensitivity;
  int res = hid_send_feature_report(handle, buf, 2);
  if (res < 0) {
    return 0;
  }
  return 1;
}

int get_sensitivity(hid_device *handle) {
  unsigned char buf[256];
  buf[0] = REPORT_MXT_SENSITIVITY;
  int res = hid_get_feature_report(handle, buf, 2);
  if (res < 0) {
    return 0;
  }
  return buf[1];
}

int get_backlight(hid_device *handle) {
  unsigned char buf[256];
  buf[0] = REPORT_BACKLIGHT;
  int res = hid_get_feature_report(handle, buf, 3);
  if (res < 0) {
    return -1;
  }
  return buf[1];
}

int set_backlight(hid_device *handle, uint8_t backlight, uint8_t save) {
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

int get_moduleID(hid_device *handle) {
  unsigned char buf[256];
  buf[0] = REPORT_MODULEID;
  int res = hid_get_feature_report(handle, buf, 3);
  if (res < 0) {
    return -1;
  }
  return buf[1];
}

uint16_t get_customID(hid_device *handle) {
  unsigned char buf[256];
  buf[0] = REPORT_CUSTOMID;
  int res = hid_get_feature_report(handle, buf, 3);
  if (res < 0) {
    return -1;
  }
  return buf[1] | (buf[2] << 8);
}

void rotate_touch(hid_device *device, char *argv[], int start_index) {
  if (checkhtt(device)) {
    for (int i = 0; i < 4; i++) {
      if (strcmp(argv[start_index + 1], Rotation[i]) == 0) {
        int success = set_rotation(device, i);
        printf("Setting rotation to %s : %s\n", Rotation[i],
               success ? "Success!" : "Failed.");
        return;
      }
    }
    printf("Invalid parameter for sensitivity : %s\n", argv[start_index + 1]);
  }
}

void sensitivty(hid_device *device, char *argv[], int start_index) {
  if (checkhtt(device)) {
    bool validDevice = false;
    validDevice |= get_driver(device) == TOUCH_MXTxx;
    validDevice |= get_driver(device) == TOUCH_GT9xx;
    if (!validDevice) {
      printf("Setting sensitivty not supported on %s driver",
             TouchTypes[get_driver(device)]);
      return;
    }
    for (int i = 0; i < 3; i++) {
      if (strcmp(argv[start_index + 1], Sensitivity[i]) == 0) {
        int success = set_sensitivity(device, i);
        printf("Setting sensitivity to %s : %s\n", Sensitivity[i],
               success ? "Success!\nPlease reconnect the USB cable to load the "
                         "new settings.\n"
                       : "Failed.\n");
        return;
      }
    }
    printf("Invalid parameter for rotatetouch : %s\n", argv[start_index + 1]);
  }
}

void savecalibration(hid_device *device, char *argv[], int start_index) {
  if (checkhtt(device)) {
    if (get_driver(device) != TOUCH_RESISTIVE) {
      printf("Saving calibration matrix is not supported on %s driver",
             TouchTypes[get_driver(device)]);
      return;
    }
    unsigned char buffer[80];
    if (get_calmatrix(device, buffer, sizeof(buffer)) == 56) {
      FILE *f = fopen(argv[start_index + 1], "wb");
      if (!f) {
        printf("error opening %s\n", argv[start_index + 1]);
        return;
      }
      fwrite(buffer, 1, 56, f);
      fclose(f);
      printf("Calibration matrix written to %s\n", argv[start_index + 1]);
    } else {
      printf("Error retrieving calibration matrix.");
    }
  }
}

void loadcalibration(hid_device *device, char *argv[], int start_index) {
  if (checkhtt(device)) {
    if (get_driver(device) != TOUCH_RESISTIVE) {
      printf("Saving calibration matrix is not supported on %s driver",
             TouchTypes[get_driver(device)]);
      return;
    }
    unsigned char buffer[80];
    FILE *f = fopen(argv[start_index + 1], "rb");
    if (!f) {
      printf("error opening %s\n", argv[start_index + 1]);
      return;
    }

    fseek(f, 0, SEEK_END);
    int file_size = ftell(f);
    if (file_size != 56) {
      fclose(f);
      printf("File size mismatch, %s is %d bytes, expected 56.\n",
             argv[start_index + 1], file_size);
    }
    fseek(f, 0, SEEK_SET); // seek back to beginning of file
    fread(buffer, 1, 56, f);
    fclose(f);
    if (set_calmatrix(device, buffer, file_size)) {
      printf("Calibration matrix written to unit\nPlease reconnect the USB "
             "cable to load the new settings.\n");
    } else {
      printf("Error saving calibration matrix.");
    }
  }
}

int set_motorduration(hid_device *handle, uint8_t duration) {
  unsigned char buf[256];
  buf[0] = REPORT_MOTOR;
  buf[1] = duration;
  int res = hid_send_feature_report(handle, buf, 2);
  if (res < 0) {
    return 0;
  }
  return 1;
}

int set_piezoduration(hid_device *handle, uint8_t duration) {
  unsigned char buf[256];
  buf[0] = REPORT_PIEZO;
  buf[1] = duration;
  int res = hid_send_feature_report(handle, buf, 2);
  if (res < 0) {
    return 0;
  }
  return 1;
}

int set_touchfeedback(hid_device *handle, uint8_t setting) {
  unsigned char buf[256];
  buf[0] = REPORT_TOUCHFEEDBACK;
  buf[1] = setting;
  int res = hid_send_feature_report(handle, buf, 2);
  if (res < 0) {
    return 0;
  }
  return 1;
}

int set_touchdim(hid_device *handle, uint8_t setting, uint16_t timeout) {
  unsigned char buf[256];
  buf[0] = REPORT_TOUCHDIM;
  buf[1] = setting;
  buf[2] = ((timeout & 0xFF00) >> 8);
  buf[3] = (timeout & 0xFF);
  int res = hid_send_feature_report(handle, buf, 4);
  if (res < 0) {
    return 0;
  }
  return 1;
}

int capcalibrate(hid_device *handle) {
  unsigned char buf[256];
  buf[0] = REPORT_PCAPCALIBRATE;
  buf[1] = 0;
  int res = hid_send_feature_report(handle, buf, 2);
  if (res < 0) {
    return 0;
  }
  return 1;
}

void help(hid_device *device, char *argv[], int start_index) {
  printf("Usage: htt_util [options]\n\n");
  printf("options:\n");
  printf(" --help\n");
  printf("    Show help (this message).\n\n");
  printf(" --device [id]\n");
  printf("    Selects the target for the following commands in setups where "
         "multiple HTTs\n");
  printf("    are connected. (0 = default)\n\n");
  printf(" --loadcalibration [filename]\n");
  printf("    Load the calibration data from a file, only available on "
         "resistive\n");
  printf("    touch screens.\n\n");
  printf(" --rotatetouch [degrees]\n");
  printf("    Sets and saves the rotation for the touch panel (visual output "
         "will not\n");
  printf("    change orientation.) Normally the host OS should take care of "
         "screen\n");
  printf("    rotation, if the host OS does not support this, this options "
         "offers\n");
  printf("    the option to apply the rotation on the device\n");
  printf("    degrees can be [0, 90, 180, 270]. \n");
  printf("    \n");
  printf(" --savecalibration [filename]\n");
  printf("    Save the calibration data to a file, only available on resistive "
         "touch\n");
  printf("    screens.\n\n");
  printf(" --scan\n");
  printf("    Scan for HTT modules and display their settings.\n\n");
  printf(" --sensitivity [level]\n");
  printf("    Sets the sensitivity of the touch panel.\n");
  printf("    This setting is only available on mxt and 7\" gt9xx driver based "
         "modules.\n");
  printf("    Attempting set this option to anything besides 'normal' on any "
         "other product\n");
  printf("    will lead to undefined behavior and is not recommended.\n\n");
  printf("    level for mxt can be [normal,high,extra]\n");
  printf("    level for 7\" GT9xx can be [normal,high]\n\n\n");
  printf("===Following commands are for PCB Rev 2.0 or higher only====\n\n");
  printf(" --backlight [setting]\n");
  printf("    set backlight brightness (PWM setting)  [0-255]\n\n");
  printf(" --backlightset [setting]\n");
  printf("    set and save backlight brightness (PWM setting)  [0-255]\n\n");
  printf(" --motor [duration]\n");
  printf("    set duration for motor on for 400Hz (in 100ms increments)\n");
  printf("    for 1 second - put [10] \n\n");
  printf(" --piezo [duration]\n");
  printf("    set duration for piezo on for 400Hz (in 100ms increments)\n");
  printf("    for 1 second - put [10] \n\n");
  printf(" --touchfeedback\n");
  printf("    set touch feedback: [0 none, 1 motor, 2 piezo, 3 motor and "
         "piezo].\n\n");
  printf(" --touchdim [BL setting]\n");
  printf("    [0-255] [0-600] \n");
  printf("    dim to [BL setting] after [time out delay in 1s increment] from "
         "last touch\n");
  printf("    to disable feature: --touchdim 0 0\n\n");
  printf(" --capcalibrate \n");
  printf("    PCAP calibrate\n");
}

void scan_internal(hid_device *handle, int index) {
  if (checkhtt(handle)) {
    int driver = get_driver(handle);
    int fwrev = get_fwrev(handle);
    printf("HTT Detected.\n");
    printf("- Device            : %d\n", index);
    printf("- Firmware Rev      : %d\n", fwrev);
    printf("- Driver Type       : %s\n", TouchTypes[driver]);
    printf("- Screen Rotation   : %s degrees\n",
           Rotation[get_rotation(handle)]);
    if (driver == TOUCH_MXTxx || driver == TOUCH_GT9xx) {
      int sens = get_sensitivity(handle);
      printf("- Touch Sensitivity : %d (%s).\n", sens, Sensitivity[sens]);
    }
    if (g_verbose && fwrev > 10656) {
      printf("- Module ID         : %d\n", get_moduleID(handle));
      uint16_t customID = get_customID(handle);
      printf("- Custom ID         : %4x\n", customID);
    }
    printf("\n");
  }
}

void scan(hid_device *handle, char *argv[], int start_index) {
  for (size_t i = 0; i < g_device_count; i++) {
    scan_internal(g_handles[i], i);
  }
}

void verbose(hid_device *handle, char *argv[], int start_index) {
  g_verbose = 1;
}

void do_brightness(hid_device *device, char *argv[], int start_index,
                   int save) {
  if (checkhtt(device)) {
    int brightness = atoi(argv[start_index + 1]);
    if (brightness > 255)
      brightness = 255;
    if (brightness < 0)
      brightness = 0;
    int success = set_backlight(device, brightness, save);
    printf("Setting brightness to %d : %s\n", brightness,
           success ? "Success!" : "Failed.");
    return;
  }
}

void brightness(hid_device *device, char *argv[], int start_index) {
  do_brightness(device, argv, start_index, 0);
}

void brightnessset(hid_device *device, char *argv[], int start_index) {
  do_brightness(device, argv, start_index, 1);
}

void select_device(hid_device *device, char *argv[], int start_index) {
  size_t devid = atoi(argv[start_index + 1]);
  if (devid < g_device_count) {
    g_currentDevice = devid;
    printf("Device %d selected.\n", devid);
  } else {
    printf("Invalid device ID : %d\n", devid);
  }
}

void motorduration(hid_device *device, char *argv[], int start_index) {
  if (checkhtt(device)) {
    int duration = atoi(argv[start_index + 1]);
    if (duration > 100)
      duration = 100;
    if (duration < 0)
      duration = 0;
    int success = set_motorduration(device, duration);
    printf("Setting motor duration to %d : %s\n", duration,
           success ? "Success!" : "Failed.");
    return;
  }
}

void piezoduration(hid_device *device, char *argv[], int start_index) {
  if (checkhtt(device)) {
    int duration = atoi(argv[start_index + 1]);
    if (duration > 100)
      duration = 100;
    if (duration < 0)
      duration = 0;
    int success = set_piezoduration(device, duration);
    printf("Setting piezo duration to %d : %s\n", duration,
           success ? "Success!" : "Failed.");
    return;
  }
}

void touchfeedback(hid_device *device, char *argv[], int start_index) {
  if (checkhtt(device)) {
    int setting = atoi(argv[start_index + 1]);
    if (setting > 3)
      setting = 3;
    if (setting < 0)
      setting = 0;
    int success = set_touchfeedback(device, setting);
    printf("Setting touch feedback to %d : %s\n", setting,
           success ? "Success!" : "Failed.");
    return;
  }
}

void touchdim(hid_device *device, char *argv[], int start_index) {
  if (checkhtt(device)) {
    int setting = atoi(argv[start_index + 1]);
    if (setting > 255)
      setting = 255;
    if (setting < 0)
      setting = 0;
    int timeout = atoi(argv[start_index + 2]);
    if (timeout > 600) {
      timeout = 600;
      printf("Maximum timeout delay is 600 (10 minutes)\n");
    }
    if (timeout < 0)
      timeout = 0;
    int success = set_touchdim(device, setting, timeout);
    printf("Setting touch dimming properties to %d, %ds delay timeout : %s\n",
           setting, timeout, success ? "Success!" : "Failed.");
    return;
  }
}

void pcapcalibrate(hid_device *device, char *argv[], int start_index) {
  if (checkhtt(device)) {
    int success = capcalibrate(device);
    printf("Calibrate: %s\n", success ? "Success!" : "Failed.");
    return;
  }
}

cli_parm handlers[] = {{"--help", 1, help},
                       {"--scan", 1, scan},
                       {"--verbose", 1, verbose},
                       {"--rotatetouch", 2, rotate_touch},
                       {"--sensitivity", 2, sensitivty},
                       {"--savecalibration", 2, savecalibration},
                       {"--loadcalibration", 2, loadcalibration},
                       {"--backlight", 2, brightness},
                       {"--backlightset", 2, brightnessset},
                       {"--device", 2, select_device},
                       {"--motor", 2, motorduration},
                       {"--piezo", 2, piezoduration},
                       {"--touchfeedback", 2, touchfeedback},
                       {"--touchdim", 3, touchdim},
                       {"--capcalibrate", 1, pcapcalibrate}};

int main(int argc, char *argv[]) {
  if (hid_init()) {
    printf("Error initializing USB\n");
    return -1;
  }

  hid_device_info *device, *iterator;
  device = iterator = hid_enumerate(0x1b3d, 0x14c9);
  /* count the number of devices available */
  while (iterator) {
    g_device_count++;
    iterator = iterator->next;
  }

  /* allocate ram for them */
  g_handles = (hid_device **)calloc(g_device_count + 1, sizeof(void *));

  /* store handles for all devices*/
  iterator = device;
  int curdevice = 0;
  while (iterator) {
    g_handles[curdevice++] = hid_open_path(iterator->path);
    iterator = iterator->next;
  }

  if (argc == 1) {
    help(NULL, NULL, 0);
  } else {
    for (size_t i = 1; i < (size_t)argc;) {
      bool work_done = false;
      size_t table_size = sizeof(handlers) / sizeof(cli_parm);
      for (size_t j = 0; j < table_size; j++) {
        if (strcmp(handlers[j].name, argv[i]) == 0) {
          if (i + handlers[j].parameter_count <= (size_t)argc) {
            handlers[j].handler(g_handles[g_currentDevice], argv, i);
            i += handlers[j].parameter_count;
            work_done = true;
            break;
          } else {
            printf("missing parameter(s) for option : %s\n", argv[i]);
            break;
          }
        }
      }
      if (!work_done) {
        printf("Unknown parameter %s\n", argv[i]);
        break;
      }
    }
  }
  for (size_t i = 0; i < g_device_count; i++) {
    hid_close(g_handles[i]);
  }
  free(g_handles);
  hid_exit();
  return 0;
}
