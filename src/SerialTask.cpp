/*
 * SerialTask.cpp
 *
 *  Created on: Mar 15, 2016
 *      Author: min
 */

//#include <iostream>
//using namespace std;

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint-gcc.h>	//for uint8_t

#include <time.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>   //for bzero()
#include <sys/uio.h>  //for readv()
#include <sys/ioctl.h>

#include <signal.h>
#include <sys/select.h>
#include <inttypes.h> //for sscanf( SCNx64 )

#include "SerialTask.h"
#include "SQLTask.h"

//Static initialize
bool SerialTask::is_Signal_Timer = false;
bool SerialTask::is_Select_key   = false;

/*********************************************************************/
void time_signal_handler(int signum)
{
	//FOR TEST
	//printf("time_signal_handler(SIGRTMIN):\n");
	//SerialTask::set_test();
	SerialTask::set_is_signal_timer(true);

}
/*********************************************************************
void io_signal_handler(int status)
{
	//FOR TEST
	printf("io_signal_handler(SIGIO): ");

	//SerialTask serial;
	//serial.set_is_signal_io(true);

}
**********************************************************************/




SerialTask::SerialTask() {

	this->exitOut           = false;
	//this->is_Select_IO      = false;
	this->is_Select_Timeout = false;
	this->is_Select_Noinput = false;
	this->is_ready_readSerial = false;
	this->is_ready_writeSerial= false;
	this->is_ready_updateSQL= false;
	this->is_ready_cmdfile  = false;
	this->keyInputState     = KEY_IDLE;
	this->inBufferState     = INBUFF_IDLE;
	this->inBuffer = new uint8_t [INOUT_BUFF_SIZE];
	this->inBufferPtr       = 0;
	this->inBufferCsum		= 0;

	this->appMsg = new AppMessage_t();
	this->cmdData = new CommandData_t();
	cmdData->newpid = 0;
/*
	this->p_nodeType  = 0;
	this->p_shortAddr = 0x0000;
	this->p_groupAddr = 0x0100;
	this->p_version   = 0x0101;
	this->p_panid     = 0x1234;
	this->p_channel    = 0x0f;
	this->p_status    = 0x00;
	this->p_maxVal    = 0x01;
	this->p_minVal    = 0x02;

	this->gProductId  = 0;
	this->gCommandId  = 0;
	this->gDataPID    = 0;
	this->gData0      = 0;
	this->gData1      = 0;
*/
}

SerialTask::~SerialTask() {
	delete[] inBuffer;
	delete appMsg;
	delete cmdData;
}

/*******************************************************/
void SerialTask::set_is_signal_timer(bool tf){ SerialTask::is_Signal_Timer = tf;}
bool SerialTask::get_is_signal_timer(void){ return (SerialTask::is_Signal_Timer); }
void SerialTask::set_is_select_key(bool tf){ SerialTask::is_Select_key = tf;}
bool SerialTask::get_is_select_key(void){return SerialTask::is_Select_key;}

void SerialTask::set_exitOut(void) {	this->exitOut = true; }
bool SerialTask::get_exitOut(void) {	return (this->exitOut); }
//void SerialTask::set_is_select_io(bool tf){	this->is_Select_IO = tf;}
//bool SerialTask::get_is_select_io(void){return (this->is_Select_IO); }
void SerialTask::set_is_select_timeout(bool tf) {this->is_Select_Timeout = tf; }
bool SerialTask::get_is_select_timeout(void){ return (this->is_Select_Timeout); }
void SerialTask::set_is_select_noinput(bool tf) {this->is_Select_Noinput = tf; }
bool SerialTask::get_is_select_noinput(void){ return (this->is_Select_Noinput); }

void SerialTask::set_is_ready_readSerial(bool tf){this->is_ready_readSerial = tf;}
bool SerialTask::get_is_ready_readSerial(void){return this->is_ready_readSerial;}
void SerialTask::set_is_ready_writeSerial(bool tf){this->is_ready_writeSerial = tf;}
bool SerialTask::get_is_ready_writeSerial(void){return this->is_ready_writeSerial;}
void SerialTask::set_is_ready_updateSQL(bool tf){this->is_ready_updateSQL = tf;}
bool SerialTask::get_is_ready_updateSQL(void){return this->is_ready_updateSQL;}
void SerialTask::set_is_ready_cmdfile(bool tf){	this->is_ready_cmdfile = tf;}
bool SerialTask::get_is_ready_cmdfile(void){return this->is_ready_cmdfile;}





/*******************************************************/
void SerialTask::errExit(FILE *file,std::string str)
{
	char *msg = &str[0];
	fprintf(file,msg);
	this->printTimeStamp(file);
	//serialClose();
	exit(EXIT_FAILURE);
}


/*********************************************************************/
void SerialTask::printTimeStamp(FILE *file)
{
	time_t t = time(NULL);
	struct tm mytm = *localtime(&t);
	fprintf(file,"TIME : %d.%d.%d. %d:%d:%d\n",
		mytm.tm_mon+1,mytm.tm_mday,mytm.tm_year+1900,
		mytm.tm_hour,mytm.tm_min,mytm.tm_sec );
}

/*********************************************************************/
void SerialTask::startMessage(FILE *file)
{
	fprintf(file,"=================================\n");
	fprintf(file,"Start GateWay Server\n");
	//fprintf(file,"Time      :");
	this->printTimeStamp(file);
	fprintf(file,"Author : Min Gwak\n");
	fprintf(file,"Verion : 0.3.1 \n");
	fprintf(file,"=================================\n");
}


/*********************************************************************/
void SerialTask::init_select(int state)
{
	struct termios ttystate;
	//get terdminal state
	tcgetattr(STDIN_FILENO,&ttystate);
	if(state == NB_ENABLE){
		//turn off canonical mode
		ttystate.c_lflag &= ~ICANON;
		ttystate.c_cc[VMIN] = 3;

	}else if(state == NB_DISABLE){
		//turn on canonical mode
		ttystate.c_lflag |= ICANON;
	}
	tcsetattr(STDIN_FILENO,TCSANOW,&ttystate);
}

/*********************************************************************/
void SerialTask::init_signal(FILE *file)
{
	int retVal;

	//struct sigaction saio;
	struct sigaction satimer;
	struct sigevent  sevtimer;
	struct itimerspec itspec;
	sigset_t timer_mask;
	timer_t timerid;

	//ttyACM_number = 0;
	//Dynamic memory allocation
	//appMsg = (struct AppMessage*)malloc(sizeof(struct AppMessage) );
	//appCmd = (struct AppCommand*)malloc(sizeof(struct AppCommand) );

	/*=============================================SIGIO
	saio.sa_handler = io_signal_handler;
	sigemptyset(&saio.sa_mask);
	saio.sa_flags = 0;
	saio.sa_restorer = NULL;
	retVal = sigaction(SIGIO,&saio,NULL);
	if(retVal == -1){
		//errorNum = ERROR_SIGIO;
		errExit("Error:sigaction(saio)\n");
	}
	==================================================*/

	/*==========================================SIGRTMIN**/
	satimer.sa_handler = time_signal_handler;
	//satimer.sa_sigaction = time_signal_handler;
	sigemptyset(&satimer.sa_mask);
	satimer.sa_flags = SA_SIGINFO;
	retVal = sigaction(SIGRTMIN,&satimer,NULL);
	if(retVal < 0){
		//errorNum = ERROR_SIGTIMER;
		errExit(file,"init_signal():sigaction(satimer):ERROR-");
	}
	//Blocking signal for timer setting -------------------------
	sigemptyset(&timer_mask);
	sigaddset(&timer_mask,SIGRTMIN);
	retVal = sigprocmask(SIG_SETMASK,&timer_mask,NULL);
	if(retVal < 0){
		//errorNum = ERROR_SIGTIMER;
		errExit(file,"init_signal():sigprocmask(setmask):ERROR-");
	}
	//Create the timer
	sevtimer.sigev_notify = SIGEV_SIGNAL;
	sevtimer.sigev_signo  = SIGRTMIN;
	sevtimer.sigev_value.sival_ptr = &timerid;
	retVal = timer_create (CLOCK_REALTIME,&sevtimer,&timerid);
	if(retVal < 0){
		//errorNum = ERROR_SIGTIMER;
		errExit(file,"init_signal():timer_create():ERROR-");
	}
	//printf("timer ID:0x%lx\n",(long)timerid);
	//start timer
	itspec.it_value.tv_sec  = 1;
	itspec.it_value.tv_nsec = 0;
	itspec.it_interval.tv_sec  = TIMER_INTERVAL_SEC;
	itspec.it_interval.tv_nsec = 0;
	retVal = timer_settime (timerid, 0, &itspec, NULL);
	if(retVal < 0){
		//errorNum = ERROR_SIGTIMER;
		errExit(file,"init_signal():timer_settime():ERROR-");
	}
	//UNBlocking signal for timer notification can be delivered
	retVal = sigprocmask(SIG_UNBLOCK,&timer_mask,NULL);
	if(retVal < 0){
		//errorNum = ERROR_SIGTIMER;
		errExit(file,"init_signal():sigprocmask(unblcok):ERROR-");
	}
	//printf("Unblocking timer-signal(%d)\n",SIGRTMIN);
	/**===================================================*/
}

/*********************************************************************/
void SerialTask::close_serial(int afd){	close(afd); }

/*********************************************************************/
int SerialTask::open_serial(FILE *file,char *atty)
{
	int fd;
	int retVal;
	struct termios options;
	//-//struct sigaction saio;

	/** 1. open ttyACM ***/
	fd = open(atty, O_RDWR|O_NOCTTY|O_NDELAY|O_TRUNC);
	// O_RDWR : Read/Write
	// O_NOCTTY : Doesn't want to 'controlling terminal'
	// O_NDELAY : Doesn't care state of the DCD signal

	if(fd < 0)	{
		errExit(file,"open_serial():Unable to Open:");
	}
	//fprintf(stdout,"serialOpen(%s)\n",atty);

	fcntl(fd,F_SETFL,0);

	/**SIGIO***************************************
	//2. Set Signal handler before make device asynchronous
	//-//saio.sa_handler = io_signal_handler;
	//-//saio.sa_mask = 0;
	//-//saio.sa_flags = 0;
	//-//saio.sa_restorer = NULL;
	//-//sigaction(SIGIO,&saio,NULL);

	//3. allow the process to receive SIGIO
	fcntl(fd,F_SETOWN,getpid());
	//-make the fileDesc asynchronous
	//-only O_APPEND and O_NONBLOCK will work with F_SETFL
	fcntl(fd,F_SETFL,FASYNC);
	*********************************************/



	/** 4. Set Configurations  **/
	tcgetattr(fd,&options);
	bzero(&options,sizeof(options));
	options.c_cflag = (B115200|CRTSCTS|CS8|CLOCAL|CREAD);
	options.c_iflag = IGNPAR;
	options.c_oflag = 0;
	options.c_lflag = 0;
	options.c_cc[VTIME] = 0;	//<---------------------------------------
	options.c_cc[VMIN]  = 30;	//blocking until minimum 30 chars received
	tcflush(fd,TCIFLUSH);

	retVal = tcsetattr(fd,TCSANOW, &options);
	if(retVal != 0)	{
		close(fd);
		errExit(file,"open_serial():Unable to set attribute:");

	}//else{
	//	printf("serialOpen():Initialized\n");
	//}

	return fd;
}

/********************************************************************/
int SerialTask::readCmdfile(FILE *file)
{
	size_t retVal = -1;
	FILE *cmdfile;
	cmdfile = fopen("commands.bin","rb+");
	if (!cmdfile){
		fprintf(file,"read Command File():Cannot open:ERROR-");
		this->printTimeStamp(file);
	}else{
		retVal = fread(cmdData,sizeof(CommandData_t),1,cmdfile);
		fprintf(file,"readCmdFile[%d]\n",retVal);
	}
	fclose(cmdfile);
	return (int)retVal;
}
int SerialTask::writeCmdfile(FILE *file)
{
	size_t retVal = -1;
	FILE *cmdfile;
	cmdfile = fopen("commands.bin","wb+");
	if (!cmdfile){
		fprintf(file,"write Command File():Cannot open:ERROR-");
		this->printTimeStamp(file);
	}else{
		retVal = fwrite(cmdData,sizeof(CommandData_t),1,cmdfile);
		fprintf(file,"writeCmdFile[%d]\n",retVal);
	}
	fclose(cmdfile);
	return (int)retVal;
}

/*********************************************************************/
void SerialTask::keyCommand(FILE *infile,FILE *outfile)
{
	int retVal;
	char keyinput[3];
	char key_cmd[] = "cmd";

	switch(this->keyInputState){
	case KEY_IDLE:
		fscanf(infile,"%s",keyinput);
		if( strcmp(keyinput,key_cmd) == 0 ){
			fprintf(outfile,"> Enter CommandID:");
			this->keyInputState = KEY_CMDID;
		}else{
			fprintf(outfile,"> Wrong Command !!\n");
		}
		break;
	case KEY_CMDID:
		fscanf(infile,"%hhx",&(cmdData->commandid) );
		fprintf(outfile,"> Enter ProductID:");
		this->keyInputState = KEY_PID;
		break;

	case KEY_PID:
		fscanf(infile,"%llx",&(cmdData->productid) );

		switch(cmdData->commandid){
		case CMD_ID_PRODUCTID:
		case CMD_ID_SET_NETWORK:
		case CMD_ID_SETCONFIG:
			fprintf(outfile,"> Enter first DATA:");
			this->keyInputState = KEY_DATA0;
			break;
		default:
			//------------------------------------------------------
			retVal = this->writeCmdfile(outfile);
			this->set_is_ready_writeSerial(true);
			/****
			retVal = this->readCmdfile();
			if(retVal < 0){
				fprintf(outfile,"readCmdFile(binary)Error!!\n");
			}
			//-----------------------------------------------------
			retVal = this->write_serial(logfile,fd);
			if(retVal < 0){
				fprintf(outCommand,"write_serial()Error!!\n");
			}
			****/
			this->keyInputState = KEY_IDLE;
		}
		break;
	case KEY_DATA0:
		if(cmdData->commandid == CMD_ID_PRODUCTID ){
			fscanf(infile,"%llx",&(cmdData->newpid ) );
			retVal = this->writeCmdfile(outfile);
			this->set_is_ready_writeSerial(true);
			//------------------------------------------------------
			//retVal = this->writeCmdfile();
			//if(retVal < 0){
			//	fprintf(logfile,"writeCmdFile(binary)Error!!\n");
			//}
			//retVal = this->readCmdfile();
			//if(retVal < 0){
			//	fprintf(logfile,"readCmdFile(binary)Error!!\n");
			//}
			//-----------------------------------------------------
			//retVal = this->write_serial(logfile,fd);
			//if(retVal < 0){
			//	fprintf(outCommand,"write_serial()Error!!\n");
			//}
			this->keyInputState = KEY_IDLE;
		}else if(cmdData->commandid == CMD_ID_SETCONFIG ){
			fscanf(infile,"%hhx",&(cmdData->conf.status) );
			fprintf(outfile,"> Enter maxVal:");
			this->keyInputState = KEY_DATA1;
		}else if(cmdData->commandid == CMD_ID_SET_NETWORK){
			fscanf(infile,"%hx",&(cmdData->netconf.shortAddr));
			fprintf(outfile,"> Enter groupAddr:");
			this->keyInputState = KEY_DATA1;
		}else{
			this->keyInputState = KEY_IDLE;
		}
		break;
	case KEY_DATA1:
		if(cmdData->commandid == CMD_ID_SETCONFIG ){
			fscanf(infile,"%hhx",&(cmdData->conf.maxVal) );
			fprintf(outfile,"> Enter minVal:");
			this->keyInputState = KEY_DATA2;
		}else if(cmdData->commandid == CMD_ID_SET_NETWORK){
			fscanf(infile,"%hx",&(cmdData->netconf.groupAddr));
			fprintf(outfile,"> Enter softVersion:");
			this->keyInputState = KEY_DATA2;
		}else{
			this->keyInputState = KEY_IDLE;
		}
		break;
	case KEY_DATA2:
		if(cmdData->commandid == CMD_ID_SETCONFIG ){
			fscanf(infile,"%hhx",&(cmdData->conf.minVal) );
			retVal = this->writeCmdfile(outfile);
			this->set_is_ready_writeSerial(true);
			//------------------------------------------------------
			//retVal = this->writeCmdfile();
			//if(retVal < 0){
			//	fprintf(logfile,"writeCmdFile(binary)Error!!\n");
			//}
			//retVal = this->readCmdfile();
			//if(retVal < 0){
			//	fprintf(logfile,"readCmdFile(binary)Error!!\n");
			//}
			//-----------------------------------------------------
			//retVal = this->write_serial(logfile,fd);
			//if(retVal < 0){
			//	fprintf(outCommand,"write_serial()Error!!\n");
			//}
			this->keyInputState = KEY_IDLE;
		}else if(cmdData->commandid == CMD_ID_SET_NETWORK){
			fscanf(infile,"%hx",&(cmdData->netconf.softVersion));
			fprintf(outfile,"> Enter panId:");
			this->keyInputState = KEY_DATA3;
		}else{
			this->keyInputState = KEY_IDLE;
		}
		break;
	case KEY_DATA3:
		if(cmdData->commandid == CMD_ID_SET_NETWORK){
			fscanf(infile,"%hx",&(cmdData->netconf.panId));
			fprintf(outfile,"> Enter channel:");
			this->keyInputState = KEY_DATA4;
		}else{
			this->keyInputState = KEY_IDLE;
		}
		break;
	case KEY_DATA4:
		if(cmdData->commandid == CMD_ID_SET_NETWORK){
			fscanf(infile,"%hhx",&(cmdData->netconf.myChannel));
			fprintf(outfile,"> Enter nodeType:");
			this->keyInputState = KEY_DATA5;
		}else{
			this->keyInputState = KEY_IDLE;
		}
		break;
	case KEY_DATA5:
		if(cmdData->commandid == CMD_ID_SET_NETWORK ){
			fscanf(infile,"%hhx",&(cmdData->netconf.nodeType) );
			retVal = this->writeCmdfile(outfile);
			this->set_is_ready_writeSerial(true);
			//------------------------------------------------------
			//retVal = this->writeCmdfile();
			//if(retVal < 0){
			//	fprintf(logfile,"writeCmdFile(binary)Error!!\n");
			//}
			//retVal = this->readCmdfile();
			//if(retVal < 0){
			//	fprintf(logfile,"readCmdFile(binary)Error!!\n");
			//}
			//-----------------------------------------------------
			//retVal = this->write_serial(logfile,fd);
			//if(retVal < 0){
			//	fprintf(outCommand,"write_serial()Error!!\n");
			//}
		}
		this->keyInputState = KEY_IDLE;
		break;
	}// End of switch

}

/*********************************************************************/
int SerialTask::select_event( int afd)
{
	//------------------------select
	//int *tmpVal;
	int retVal;	//,retWrite;
	int max_fd;
	int fd = afd;

	fd_set input;
	struct timeval timeout;
	timeout.tv_sec = TIMER_INTERVAL_SEC;			//120 sec time out
	timeout.tv_usec = 0;

	FD_ZERO(&input);
	FD_SET(STDIN_FILENO,&input);	//set key-input
	FD_SET(fd,&input);				//set serial input

	max_fd = ( (fd > STDIN_FILENO)? fd :STDIN_FILENO ) + 1;
	retVal = select(max_fd,&input,NULL,NULL,&timeout);
	//retVal = select(max_fd,&input,NULL,NULL,NULL);
	if(retVal < 0){
		//fprintf(outfile,"select_event(no-input)\n");
		this->set_is_select_noinput(true);
	}else if(retVal == 0){
		//fprintf(logfile,"select_event(%d sec-timeout):\n",TIMER_INTERVAL_SEC);
		this->set_is_select_timeout(true);
		timeout.tv_sec = TIMER_INTERVAL_SEC;
		timeout.tv_usec = 0;
	}else{
		if(FD_ISSET(fd,&input)){

			//fprintf(outfile,"select_event(serial_IO):\n");
			this->set_is_ready_readSerial(true);
			/****
			retVal = this->read_serial(logfile,fd);
			if(retVal < RETURN_OK){
				fprintf(stdout,"read_serial():ERROR-");
				this->printTimeStamp(logfile);
			}else{
				//-----FOR TEST
				//fprintf(logfile,"read_serial(): %d bytes\n",retVal);
			}
			****/
		}else if(FD_ISSET(STDIN_FILENO,&input)){

			//SerialTask::set_is_select_key(true);
			this->keyCommand(stdin,stdout);

		}//End of }else if(FD_ISSET(STDIN_FILENO,&input)){

	}	//End of }else{ (select)
	return retVal;
}

/**********************************************************************/
//int SerialTask::write_serial(int afd, uint64_t apid, uint8_t acid)
int SerialTask::write_serial(FILE *file, int afd)
{
	int fd = afd;
	int i,retVal;
	//int outBufSize;
	int outBufferPtr = 0;
	uint8_t outBuffer[INOUT_BUFF_SIZE];
	int tmpBufferPtr = 0;
	uint8_t tmpBuffer[INOUT_BUFF_SIZE];
	uint8_t outCsum;

	//uint64_t productid = this->gProductId;
	//uint8_t commandid  = this->gCommandId;
	//uint64_t new_productid = this->gDataPID;
	//uint16_t data0     = this->gData0;
	//uint16_t data1     = this->gData1;


	//int duration = 1000;	// 3000msec->0x0bb8
	//int period  = 100;		// 100time->0x0064
	//printf("> productID=0x%.16llx : commandID=0x%.2hhx :  \n",productid, commandid);
	//if((CommandID_t)(commandid) == CMD_ID_IDENTIFY){
	//	tmpBufferPtr = 13;
	//	tmpBuffer[0] = commandid;
	//	this->valueToArray(64, (void *)&(appMsg->productId), (void *)&tmpBuffer[1] );
	//	this->valueToArray(16,(void *)&duration, (void *)&tmpBuffer[9] );
	//	this->valueToArray(16,(void *)&period, (void *)&tmpBuffer[11]);
	//}else
	//------------------------------------------------------
	//retVal = this->readCmdfile();
	//if(retVal < 0){
	//	fprintf(file,"readCmdFile(binary)Error!!\n");
	//}
	//-----------------------------------------------------
	if((cmdData->commandid) == CMD_ID_SET_NETWORK){

		tmpBufferPtr = 19;
		tmpBuffer[0] = cmdData->commandid;
		this->valueToArray(64,(void *)&(cmdData->productid), (void *)&tmpBuffer[1] );
		tmpBuffer[9] = cmdData->netconf.nodeType;
		this->valueToArray(16,(void *)&(cmdData->netconf.shortAddr), (void *)&tmpBuffer[10] );
		this->valueToArray(16,(void *)&(cmdData->netconf.groupAddr), (void *)&tmpBuffer[12]);
		this->valueToArray(16,(void *)&(cmdData->netconf.softVersion), (void *)&tmpBuffer[14]);
		this->valueToArray(16,(void *)&(cmdData->netconf.panId), (void *)&tmpBuffer[16]);
		tmpBuffer[18] = (cmdData->netconf.myChannel);

	}else if((cmdData->commandid) == CMD_ID_SETCONFIG){

		tmpBufferPtr = 12;
		tmpBuffer[0] = cmdData->commandid;
		this->valueToArray(64,(void *)&(cmdData->productid), (void *)&tmpBuffer[1] );
		tmpBuffer[9] = cmdData->conf.status;
		tmpBuffer[10] = cmdData->conf.maxVal;
		tmpBuffer[11] = cmdData->conf.minVal;

	}else if((cmdData->commandid) == CMD_ID_PRODUCTID){
		tmpBufferPtr = 17;
		tmpBuffer[0] = cmdData->commandid;
		this->valueToArray(64, (void *)&(cmdData->productid), (void *)&tmpBuffer[1] );
		this->valueToArray(64, (void *)&(cmdData->newpid), (void *)&tmpBuffer[9] );
	}else {
		tmpBufferPtr = 9;
		tmpBuffer[0] = cmdData->commandid;
		this->valueToArray(64,(void *)&(cmdData->productid), (void *)&tmpBuffer[1] );

	}
	//-----------------------------------Create outBuffer[]
	outBuffer[outBufferPtr++] = 0x10;
	outBuffer[outBufferPtr++] = 0x02;
	outCsum = (0x10 + 0x02);
	for (i=0;i < tmpBufferPtr; i++){
		if(tmpBuffer[i] == 0x10){
			outBuffer[outBufferPtr++] = 0x10;
			outCsum += 0x10;
		}
		outBuffer[outBufferPtr++] = tmpBuffer[i];
		outCsum += tmpBuffer[i];
	}
	outBuffer[outBufferPtr++] = 0x10;
	outBuffer[outBufferPtr++] = 0x03;
	outCsum += (0x10 + 0x03);
	outBuffer[outBufferPtr++] = outCsum;

	// Writing Data to port
	retVal = write(fd, outBuffer, outBufferPtr);
	//fsync(fd);
	/**********************For Test
	if( retVal < 0 ){
		//fprintf(stderr, "serial_write():Can't write.\n ");
	}else{
		fprintf(stdout,"write_serial(TEST):[%d]",retVal );
		for (i=0; i < retVal; i++){
			fprintf(stdout,"%x,",outBuffer[i]);
		}
		//fprintf(stdout,"\n");
	}
	***************************/
	if(retVal < 0){
		fprintf(file,"write_serial():Error-");
		this->printTimeStamp(file);
	}else{
		//fprintf(file,"Write_serial(): %d bytes\n",retVal);
		//-------------------for test------------------------
		fprintf(file,"Write_serial(TEST):%d bytes:",retVal);
		for (i=0; i < retVal; i++){
			fprintf(stdout,"%x,",outBuffer[i]);
		}
		fprintf(stdout,"\n");
	}
	return retVal;
}

/**********************************************************************/
void SerialTask::valueToArray(int bits,void *input,void *output)
{
	uint8_t *inValue = (uint8_t *)input;
	uint8_t *outArray = (uint8_t *)output;
	int i,repeat;
	if (bits == 64) 		repeat = 8;//7;
	else if (bits == 32) 	repeat = 4;//3;
	else if (bits == 16) 	repeat = 2;//1;
	else 	repeat = 1;//0;
	//for (i = repeat; i >= 0 ;i-- ){
	for (i = 0; i < repeat ;i++ ){
		*outArray++ = *inValue++;
	}

}

/*********************************************************************/
int SerialTask::read_serial(FILE *file,int afd)
{
	int retVal,readNum,i;
	int fd = afd;
	uint8_t  tmpBuffer[TMP_BUFF_SIZE];


	ioctl(fd,FIONREAD,&readNum);
	if(readNum < 0)	{
		fprintf(file,"read_serial():Can't read:ERROR-");
		this->printTimeStamp(file);
	}else{
		//fprintf(file,"read_serial():ioctl(%d bytes):\n",readNum);
		if(readNum > TMP_BUFF_SIZE){
			tcflush(fd,TCIFLUSH);
			//TCIFLUSH: flushes the input queue
			fprintf(file,"read_serial():flushed queue:\n");
		}else{
			readNum = read(fd,tmpBuffer,readNum);
			for (i=0 ; i < readNum ; i++){
				//--------------------------For TEST
				//printf("%x,",tmpBuffer[i]);
				//----------------------------------
				retVal = this->read_buffer( tmpBuffer[i]);
				//--------for test-------------------------
				if(retVal == 0){
					fprintf(file,"read_buffer():Receive Data:");
					for (i=0 ; i < this->inBufferPtr; i++){
						fprintf(file,"%x,",this->inBuffer[i]);
					}
					fprintf(file,"\n");
				}else
				//-----------------------------------------
				if(retVal == -1){
					fprintf(file,"read_buffer():CSUM Error:");
					this->printTimeStamp(file);
				}else if(retVal == -2){
					fprintf(file,"create_appMessage():Un-known CommandID:0x%x:",this->inBuffer[0]);
					this->printTimeStamp(file);
					//for (i=0 ; i < this->inBufferPtr; i++){
					//	fprintf(file,"%x,",this->inBuffer[i]);
					//}
					//fprintf(file,"\n");
				//}else if(retVal == -3){
				//	continue working...
				}else if(retVal > 0){
					fprintf(file,"create_appMessage():Error:Data-Length= %d bytes:", retVal);
					this->printTimeStamp(file);
				}

			}
			//--------------------------For TEST
			//printf("\n");
			//----------------------------------s


				//if(retVal == 0){
				//	//fprintf(file,"read_serial(%d bytes):Success:\n",this->inBufferPtr);
				//}else if(retVal == -1){
				//	fprintf(file,"read_buffer():CSUM Error:");
				//	this->printTimeStamp(file);
				//}else if(retVal == -2){
				//	fprintf(file,"create_appMessage():Un-known CommandID:0x%x \n",this->inBuffer[0]);
				//	this->printTimeStamp(file);
				//	for (i=0 ; i < this->inBufferPtr; i++){
				//		fprintf(file,"%x,",this->inBuffer[i]);
				//	}
				//	fprintf(file,"\n");
				//}else{
				//	fprintf(file,"create_appMessage():Error:Data-Length= %d bytes:", retVal);
				//	this->printTimeStamp(file);
				//}
			//}

		}
	}
	return retVal;
}

/*********************************************************************/
int SerialTask::read_buffer(uint8_t buffer)
{

	int retVal = -3;
	switch(this->inBufferState)
	{
	case INBUFF_IDLE:
		if(0x10 == buffer){
			this->inBufferPtr = 0;
			memset(this->inBuffer,0,INOUT_BUFF_SIZE );
			this->inBufferCsum = buffer;
			this->inBufferState = INBUFF_SYNC;
		}
		retVal = -3;
		break;
	case INBUFF_SYNC:
		this->inBufferCsum += buffer;
		if(0x02 == buffer){
			this->inBufferState = INBUFF_DATA;
		}else{
			this->inBufferState = INBUFF_IDLE;
		}
		retVal = -3;
		break;
	case INBUFF_DATA:
		this->inBufferCsum += buffer;
		if(0x10 == buffer){
			this->inBufferState = INBUFF_MASK;
		}else{
			this->inBuffer[(this->inBufferPtr)++] = buffer;
			if(this->inBufferPtr >= INOUT_BUFF_SIZE){
				this->inBufferState = INBUFF_IDLE;
			}
		}
		retVal = -3;
		break;
	case INBUFF_MASK:
		this->inBufferCsum += buffer;
		if(0x10 == buffer){
			this->inBuffer[(this->inBufferPtr)++] = buffer;
			if(this->inBufferPtr >= INOUT_BUFF_SIZE){
				this->inBufferState = INBUFF_IDLE;
			}else{
				this->inBufferState = INBUFF_DATA;
			}
		}else if(0x03 == buffer){
			this->inBufferState = INBUFF_CSUM;
		}else{
			this->inBufferState = INBUFF_IDLE;
		}
		retVal = -3;
		break;
	case INBUFF_CSUM:
		if(buffer == this->inBufferCsum){
			retVal = this->create_appMessage();

		}else{
			//fprintf(file,"read_buffer():CSUM Error:");
			//this->printTimeStamp(file);
			retVal = -1;
		}
		this->inBufferState = INBUFF_IDLE;
	break;
	}

	return retVal;

}

/*********************************************************************/
int SerialTask::create_appMessage(void)
{
	//int i;
	int retVal = 0;
	//fprintf(file,"sendToDB():");
	//this->appMsg = (struct AppMessage *)malloc(sizeof(struct AppMessage) );
	//appMsg = new AppMessage();
	if ((CommandID_t)(this->inBuffer[0]) == CMD_ID_NETWORK_INFO){
		//if (this->inBufferPtr != sizeof(struct AppMessage)){//<=WRONG!!
		if (this->inBufferPtr != 30 ){
			//fprintf(file,"create_appMessage():Error:Data-Length= %d bytes:", inBufferPtr);
			//this->printTimeStamp(file);
			return (this->inBufferPtr);
		}
		this->appMsg->commandId 		= this->inBuffer[0];
		this->appMsg->nodeType  		= this->inBuffer[1];
		valueToArray(64,(void *)&this->inBuffer[2], (void *)&(this->appMsg->productId)  );
		valueToArray(16,(void *)&this->inBuffer[10],(void *)&(this->appMsg->shortAddr)  );
		valueToArray(16,(void *)&this->inBuffer[12],(void *)&(this->appMsg->groupAddr) );
		valueToArray(16,(void *)&this->inBuffer[14],(void *)&(this->appMsg->softVersion) );
		//valueToArray(32,(void *)&this->inBuffer[16],(void *)&(this->appMsg->channelMask) );
		valueToArray(16,(void *)&this->inBuffer[16],(void *)&(this->appMsg->panId) );
		this->appMsg->myChannel 		= this->inBuffer[18];
		valueToArray(16,(void *)&this->inBuffer[19],(void *)&(this->appMsg->parentAddr) );
		this->appMsg->lqi 			= this->inBuffer[21];
		this->appMsg->rssi 			= this->inBuffer[22];
		this->appMsg->config.status 	= this->inBuffer[23];
		this->appMsg->config.maxVal 	= this->inBuffer[24];
		this->appMsg->config.minVal 	= this->inBuffer[25];
		this->appMsg->data.voltage 	= this->inBuffer[26];
		this->appMsg->data.current 	= this->inBuffer[27];
		this->appMsg->data.temperature = this->inBuffer[28];
		this->appMsg->data.dim 		= this->inBuffer[29];

		//---------------------Only For Test
		//TESTprint_AppMessage();
		//---------------------------------------
		this->set_is_ready_updateSQL(true);
		retVal = 0;

	}else { 	//inBuff[0] != 0x01
		retVal = -2;
		//fprintf(file,"create_appMessage():Un-known Command ID:");
		//this->printTimeStamp(file);
		//for (i=0 ; i < this->inBufferPtr; i++){
		//	fprintf(file,"%x,",this->inBuffer[i]);
		//}
		//fprintf(file,"\n");

	}
	//free(appMsg);
	//delete appMsg;
	return retVal;
}


