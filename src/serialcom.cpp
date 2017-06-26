//============================================================================
// Name        : serialcom_acm.cpp
// Author      : min gwak
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
using namespace std;
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint-gcc.h>	//for uint8_t

#include "SerialTask.h"
#include "SQLTask.h"



SerialTask *serial;
SQLTask *sqlt;




/*********************************************************************/
int main(int argc, char* argv[])
{
	//cout << "!!!Hello World!!!" << endl; // prints !!!Hello World!!!
	int retVal;
	int fileDesc;
	FILE *logfile;
	char tty[13];
	std::string name = "sloanNet.log";
	char *filename = &name[0];

	serial = new SerialTask();
	sqlt = new SQLTask();

	//-----------------------------------------
	logfile = fopen(filename,"a+");
	if( !logfile ){
		fprintf(stderr,"Cannot open the logfile #00.\n");
		logfile = stdout;
	}
	serial->startMessage(logfile);
	fclose(logfile);
	//-----------------------------------------
	//-----------------------------------------
	logfile = fopen(filename,"a+");
	if( !logfile ){
		fprintf(stderr,"Cannot open the logfile #01.\n");
		logfile = stdout;
	}
	retVal = sqlt->init_sql(logfile);
	if(retVal != RETURN_OK){
		fprintf(logfile,"init_sql():ERROR-");
		serial->printTimeStamp(logfile);
	}else{
		fprintf(logfile,"Initialized Database\n");
	}
	fclose(logfile);
	//-----------------------------------------
	//open serial with infomation
	fprintf(stdout,"Device Location:");
	fscanf(stdin,"%s",tty);
	if( access(tty,F_OK ) < 0){
		printf("%s is not exists.",tty);
		exit(EXIT_FAILURE);
	}
	//-----------------------------------------
	logfile = fopen(filename,"a+");
	if( !logfile ){
		fprintf(stderr,"Cannot open the logfile #02.\n");
		logfile = stdout;
	}
	fileDesc = serial->open_serial(logfile,tty);
	fprintf(logfile,"Opened the port(%s)\n",tty);

	fclose(logfile);
	//------------------------------------------
	//-----------------------------------------
	logfile = fopen(filename,"a+");
	if( !logfile ){
		fprintf(stderr,"Cannot open the logfile #03.\n");
		logfile = stdout;
	}
	//init select (after open serial)
	serial->init_select(NB_DISABLE);
	fprintf(logfile,"Initialized select(event)\n");
	//init signal (after open serial)
	serial->init_signal(logfile);
	fprintf(logfile,"Initialized signal(event)\n");

	fclose(logfile);
	//------------------------------------------


	while( !serial->get_exitOut()  )
	{

		serial->select_event( fileDesc);//????

		//-----------------------------------------
		logfile = fopen(filename,"a+");
		if( !logfile ){
			fprintf(stderr,"Cannot open the logfile #04.\n");
			logfile = stdout;
		}
		//-----------------------------------------
		/***key input *******************************/
		//if(true == SerialTask::get_is_select_key()){
		//	SerialTask::set_is_select_key(false);
		//	serial->keyCommand(stdin,stdout);
		//}
		if(true == serial->get_is_ready_writeSerial()){
			serial->set_is_ready_writeSerial(false);

			retVal = serial->readCmdfile(logfile);
			retVal = serial->write_serial(logfile,fileDesc);
		}
		/*********************************************/
		if(true == serial->get_is_ready_readSerial()){
			serial->set_is_ready_readSerial(false);
			retVal = serial->read_serial(logfile,fileDesc);

			//if(retVal < RETURN_OK){
			//	fprintf(logfile,"read_serial():ERROR-");
			//	serial->printTimeStamp(logfile);
			//}else{
			//	fprintf(logfile,"read_serial(): %d bytes\n",retVal);
			//}
		}
		if(true == serial->get_is_ready_updateSQL()){
			serial->set_is_ready_updateSQL(false);
			retVal = sqlt->update_sql(logfile,serial->appMsg);
			if(retVal != RETURN_OK){
				fprintf(stdout,"update_sql():ERROR-");
				serial->printTimeStamp(stdout);
			}else{
				fprintf(stdout,"update_sql():SUCCESS\n");
			}
		}
		/*********************************************/
		if(true == SerialTask::get_is_signal_timer()){
			SerialTask::set_is_signal_timer(false);
			//fprintf(stdout,"signal Timer handle(120sec):");
			retVal = sqlt->reset_lqi(logfile);
			if(retVal != RETURN_OK){
				fprintf(logfile,"reset_lqi():ERROR-");
				serial->printTimeStamp(logfile);
			}else{
				fprintf(logfile,"reset_lqi():SUCCESS\n");
			}
			/*************************FOR TEST***/
			//retVal = sqlt->select_sql(stdout);
			//if(retVal != RETURN_OK){
			//	fprintf(stdout,"select_sql():ERROR-");
			//	serial->printTimeStamp(stdout);
			//}else{
			//	fprintf(stdout,"select_sql():SUCCESS\n");
			//}
			/*************************************/
		}
		if(true == serial->get_is_select_timeout()){
			serial->set_is_select_timeout(false);

			fprintf(logfile,"select event(%d sec timeout)\n",TIMER_INTERVAL_SEC);
		}
		if(true == serial->get_is_select_noinput()){
			serial->set_is_select_noinput(false);
			fprintf(logfile,"select event(no-input)\n");
		}
		//-----------------------------------------
		fclose(logfile);
		//-----------------------------------------
		usleep(100000);

	}
	//-----------------------------------------
	logfile = fopen(filename,"a+");
	if( !logfile ){
		fprintf(stderr,"Cannot open the logfile #05.\n");
		logfile = stdout;
	}
	//-----------------------------------------
	serial->close_serial(fileDesc);
	fprintf(logfile,"Closing serial:");
	serial->printTimeStamp(logfile);
	fclose(logfile);
	//-----------------------------------------

	delete serial;
	delete sqlt;
	return EXIT_SUCCESS;
}
