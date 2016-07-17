/*
 * config.h
 *
 *  Created on: Apr 26, 2016
 *      Author: min
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#include <stdint-gcc.h>

#define TYPE_ROUTER		0x10
#define TYPE_ENDDEVICE	0x20
enum {
	TYPE_COORDINATOR = 0x00,
	//TYPE_ROUTER      = 0x10,
	TYPE_LIGHT_DIM	 = 0x11,
	TYPE_LIGHT_COLOR = 0x12,
	//TYPE_ENDDEVICE   = 0x20,
	TYPE_SENSOR_DIM  = 0x21,
	TYPE_SENSOR_COLOR= 0x22,
};
#define DEFAULT_ADDRESS		0x1234	//APP_COORDINATOR_ADDR
#define DEFAULT_PRODTYPE	TYPE_LIGHT_DIM
#define DEFAULT_VERSION		0x0103
#define DEFAULT_PANID		0x1234	//4660
//#define DEFAULT_BAND		0x00
//#define DEFAULT_MODULATION	0x24
#define DEFAULT_CHANNEL		0x0f
#define DEFAULT_GROUPADDR	0x1000
#define DEFAULT_PARENTADDR	0x0000
#define DEFAULT_STATUS		0x01
#define DEFAULT_MAXVAL		100
#define DEFAULT_MIDVAL		75
#define DEFAULT_MINVAL		0


struct data_t{			//48bits
	uint16_t colordim;
	uint8_t sensor;		//dim,daylight,color
	uint8_t voltage;
	uint8_t current;
	uint8_t temperature;
};
struct config_t{
	uint16_t status;
	//dim    on-off/level/day-night
	//sensor on-off/level/
	uint8_t maxVal;
	uint8_t midVal;
	uint8_t minVal;
};

struct network_t{		//40bits
	uint16_t appAddr;
	uint16_t groupid;
	uint16_t parentAddr;
	uint16_t panId;
	uint8_t channel;
};
struct product_t{
	uint64_t id;
	uint16_t version;	//master.develop
	uint8_t type;
};
//--------------------------------------
typedef struct  AppSaveInfo_t {
	//uint32_t channelMask;
	struct product_t prod;
	struct network_t nwk;
	struct config_t conf;
} AppSaveInfo_t;
//-------------------------------------
typedef struct HeaderMessage_t{
	uint8_t commandId;
	uint16_t appAddr;
	uint8_t lqi;
	int8_t rssi;
} HeaderMessage_t;

typedef struct  SmallMessage_t {	//Freq sending message
	uint8_t commandId;
	uint16_t appAddr;
	uint8_t lqi;
	int8_t rssi;
	union{
		struct data_t data;
		struct config_t conf;
	};
} SmallMessage_t;

typedef struct LargeMessage_t {
	uint8_t commandId;
	uint16_t appAddr;
	uint8_t lqi;
	int8_t rssi;
	union{
		struct product_t prod;
		struct network_t nwk;
	}
} LargeMessage_t;

#endif /* CONFIG_H_ */
