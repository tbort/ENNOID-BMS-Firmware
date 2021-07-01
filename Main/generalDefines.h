// Define the hardware version here:
#ifndef ENNOID_SS_LITE
#define ENNOID_SS_LITE					0
#endif

#ifndef ENNOID_SS
#define ENNOID_SS					1
#endif

#ifndef ENNOID_LV
#define ENNOID_LV					0
#endif

#ifndef ENNOID_HV
#define ENNOID_HV					0
#endif

// Firmware version
#define FW_VERSION_MAJOR		1
#define FW_VERSION_MINOR		3
#define FW_REAL_VERSION			"V1.3"

// UUID
#define STM32_UUID					((uint32_t*)0x1FFFF7AC)
#define STM32_UUID_8				((uint8_t*)0x1FFFF7AC)

// Hardware names and limits
#if ENNOID_LV
  #define HW_NAME "ENNOID-LV"
	#define HW_LIM_CELL_BALANCE_MAX_SIMULTANEOUS_DISCHARGE 18
	#define HW_LIM_CELL_MONITOR_IC_COUNT                   18
	#define HW_LIM_MIN_NOT_USED_DELAY                      5000
#endif

#if ENNOID_HV
  #define HW_NAME "ENNOID-HV"
	#define HW_LIM_CELL_BALANCE_MAX_SIMULTANEOUS_DISCHARGE 18
	#define HW_LIM_CELL_MONITOR_IC_COUNT                   18
	#define HW_LIM_MIN_NOT_USED_DELAY                      5000
#endif

#if ENNOID_SS
  #define HW_NAME "ENNOID-SS"
	#define HW_LIM_CELL_BALANCE_MAX_SIMULTANEOUS_DISCHARGE 18
	#define HW_LIM_CELL_MONITOR_IC_COUNT                   1
	#define HW_LIM_MIN_NOT_USED_DELAY                      5000
#endif

#if ENNOID_SS_LITE
  #define HW_NAME "ENNOID-SS-LITE"
	#define HW_LIM_CELL_BALANCE_MAX_SIMULTANEOUS_DISCHARGE 18
	#define HW_LIM_CELL_MONITOR_IC_COUNT                   1
	#define HW_LIM_MIN_NOT_USED_DELAY                      5000
#endif

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
