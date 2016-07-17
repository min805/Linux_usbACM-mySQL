/*
 * SerialTask.h
 *
 *  Created on: Mar 15, 2016
 *      Author: min
 */

#ifndef SERIALTASK_H_
#define SERIALTASK_H_

#include <iostream>
using namespace std;
#include <stdint-gcc.h>	//for uint8_t

#define NB_ENABLE	0
#define NB_DISABLE  1
#define INOUT_BUFF_SIZE 50
#define TMP_BUFF_SIZE 128
#define TIMER_INTERVAL_SEC 120	//2 minutes (select-event)
#define RETURN_OK 0

typedef enum {
	INBUFF_IDLE,
	INBUFF_SYNC,
	INBUFF_DATA,
	INBUFF_MASK,
	INBUFF_CSUM,
}InBufferState_t;

typedef enum {
	KEY_IDLE,
	KEY_CMDID,
	KEY_PID,
	KEY_DATA0,
	KEY_DATA1,
	KEY_DATA2,
	KEY_DATA3,
	KEY_DATA4,
	KEY_DATA5,
}KeyInputState_t;


typedef enum {
	CMDID_REPORT_DATA = 0x01,
	CMDID_SOFTRESET   = 0x11,
	CMDID_SET_PRODUCT = 0x12,
	CMDID_SET_NETWORK = 0x13,
	CMDID_SET_CONFIG  = 0x15,
	// Include Color & Dimm level.
	//CMDID_SET_COLOR	  = 0x16,
	//CMDID_SET_DIMM    = 0x17,
	CMDID_BRIGHTDOWN  = 0x16,
	CMDID_BRIGHTUP	  = 0x17,
	CMDID_DAYLIGHT	  = 0x18,
	CMDID_NIGHTLIGHT  = 0x19,
	CMDID_IDENTIFY    = 0x1a,
	////CMDID-REQUEST 0x30
	CMDID_REQUEST_PRODUCT = 0x32,
	CMDID_REQUEST_NETWORK = 0x33,
	CMDID_REQUEST_CONFIG  = 0x34,
	////CMDID-REPORT  0x50
	CMDID_REPORT_PRODUCT  = 0x52,
	CMDID_REPORT_NETWORK  = 0x53,
	CMDID_REPORT_CONFIG   = 0x54,
	////CMDID-MULI-SET
	CMDID_MULT_SET_CONFIG = 0x95,
	CMDID_MULT_BRIGHTDOWN = 0x96,
	CMDID_MULI_BRIGHTUP   = 0x97,
	CMDID_MULT_DAYLIGHT   = 0x98,
	CMDID_MULT_NIGHTLIGHT = 0x99,
	CMDID_MULT_IDENTIFY   = 0x9a,

/******************old command*****

	CMD_ID_NETWORK_INFO = 0x01,

	CMD_ID_SOFTRESET    = 0x11,
	CMD_ID_SET_NETWORK  = 0x12,
	CMD_ID_REQUEST_INFO = 0x13,
	CMD_ID_PRODUCTID    = 0x14,

	CMD_ID_IDENTIFY     = 0x21,
	CMD_ID_SETCONFIG    = 0x22,
	CMD_ID_BRIGHTDOWN	= 0x23,
	CMD_ID_BRIGHTUP     = 0x24,

	CMD_ID_GP_IDENTIFY     = 0x31,
	CMD_ID_GP_SETCONFIG    = 0x32,
	CMD_ID_GP_BRIGHTDOWN   = 0x33,
	CMD_ID_GP_BRIGHTUP     = 0x34,
***************************************/

}CommandID_t;

//__attribute__((packed))

struct netconf_t{
	uint16_t shortAddr;
	uint16_t groupAddr;
	uint16_t softVersion;
	uint16_t panId;
	uint8_t myChannel;
	uint8_t nodeType;
};
struct config_t{
	uint8_t status;
	uint8_t maxVal;
	uint8_t minVal;
};
struct data_t{
	uint8_t voltage;
	uint8_t current;
	uint8_t temperature;
	uint8_t dim;
};

struct CommandData_t{
	uint8_t commandid;
	uint64_t productid;
	union{
		uint64_t newpid;
		struct config_t conf;
		struct netconf_t netconf;
	};
};


//AppMessage = uint8_t[34]
//10,02,AppMessage[34],10,03,CS
struct AppMessage_t{
	uint8_t commandId;
	//-------eeprom_network
	uint8_t nodeType;
	uint64_t productId;
	uint16_t shortAddr;
	uint16_t groupAddr;
	uint16_t softVersion;
	//---------------------
	//uint32_t channelMask;
	uint16_t panId;
	uint8_t myChannel;
	uint16_t parentAddr;
	uint8_t lqi;
	int8_t rssi;

	struct config_t config;
	struct data_t data;
};


class SerialTask {

public:

	SerialTask();
	virtual ~SerialTask();

	AppMessage_t *appMsg;
	CommandData_t *cmdData;

	static void set_is_signal_timer(bool tf);
	static bool get_is_signal_timer(void);
	static void set_is_select_key(bool tf);
	static bool get_is_select_key(void);

	void set_exitOut(void);
	bool get_exitOut(void);
	//void set_is_select_io(bool tf);
	//bool get_is_select_io(void);

	void set_is_select_timeout(bool tf);
	bool get_is_select_timeout(void);
	void set_is_select_noinput(bool tf);
	bool get_is_select_noinput(void);

	void set_is_ready_readSerial(bool tf);
	bool get_is_ready_readSerial(void);
	void set_is_ready_writeSerial(bool tf);
	bool get_is_ready_writeSerial(void);
	void set_is_ready_updateSQL(bool tf);
	bool get_is_ready_updateSQL(void);
	void set_is_ready_cmdfile(bool tf);
	bool get_is_ready_cmdfile(void);

	void printTimeStamp(FILE *file);
	void startMessage(FILE *file);

	void init_select(int state);
	void init_signal(FILE *file);
	int open_serial(FILE *file,char *atty);
	void close_serial(int afd);
	int select_event(int afd );
	//int write_serial(int afd, uint64_t apid, uint8_t acid);
	int write_serial(FILE *file,int afd);
	int read_serial(FILE *file,int afd );
	int readCmdfile(FILE *file);
	int writeCmdfile(FILE *file);
	void keyCommand(FILE *infile,FILE *outfile);


private:

	static bool is_Signal_Timer;
	static bool is_Select_key;

	bool exitOut;
	//bool is_Select_IO ;
	bool is_Select_Timeout;
	bool is_Select_Noinput;
	bool is_ready_readSerial;
	bool is_ready_writeSerial;
	bool is_ready_updateSQL;
	bool is_ready_cmdfile;

	KeyInputState_t keyInputState;
	InBufferState_t inBufferState;

	uint8_t inBufferCsum;
	int inBufferPtr;
	uint8_t  *inBuffer;
	//fd_set input;

	//------------------------for test
	//uint8_t p_nodeType;
	//uint16_t p_shortAddr;
	//uint16_t p_groupAddr;
	//uint16_t p_version;
	//uint16_t p_panid;
	//uint8_t p_channel;

	//uint8_t p_status;
	//uint8_t p_maxVal;
	//uint8_t p_minVal;

	//uint64_t gProductId;
	//uint8_t gCommandId;
	//uint64_t gDataPID;
	//uint16_t gData0;
	//uint16_t gData1;
	//--------------------------------

	//void time_signal_handler(int signum);
	void errExit(FILE *file,std::string str);
	void valueToArray(int bits,void *input,void *output);
	int read_buffer(uint8_t Buffer);
	int create_appMessage(void);








};



#endif /* SERIALTASK_H_ */
