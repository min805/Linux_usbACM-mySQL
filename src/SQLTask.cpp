/*
 * SQLTask.cpp
 *
 *  Created on: Mar 15, 2016
 *      Author: min
 */

#include <iostream>
using namespace std;

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <cstring>

#include </usr/sqlite3/include/sqlite3.h>
#include "SQLTask.h"
#include "SerialTask.h"


/****NOT IN SQLTask Class************************************************/
int sql_callback(void *NotUsed, int argc, char **argv, char **azColName)
{
	int i;
	fprintf(stdout,"%s\n",(const char*)NotUsed);
	for (i=0; i < argc; i++){
		fprintf(stdout,"%s = %s;",azColName[i],(argv[i]? argv[i]:"NULL") );
	}
	fprintf(stdout,"\n");
	return 0;
}




SQLTask::SQLTask() {
	//SerialTask serial = new SerialTask();

}

SQLTask::~SQLTask() {
	// TODO Auto-generated destructor stub
}


/*****************************************************/
int SQLTask::reset_lqi(FILE *file)
{
	sqlite3 *db;
	std::string str;
	char *sql;
	char *zErrMsg = 0;
	const char *data = "sql_callback call";
	int retVal;

	retVal = sqlite3_open("sloanNet.db",&db);
	if(retVal){
		//retVal != 0
		fprintf(file,"reset_lqi():Can't open database: %s\n",sqlite3_errmsg(db));
	}else{
		//sql = "update lwmTable set lqi=0, rssi=0;";
		str = "update lwmTable set lqi=0, rssi=0;";
		//sql = str.c_str();
		sql = &str[0];
		retVal = sqlite3_exec(db,sql,sql_callback,(void *)data,&zErrMsg);
		if(retVal != SQLITE_OK ){
			//retVal != 0
			fprintf(file,"reset_lqi():update-Error:%s\n",zErrMsg);
			sqlite3_free(zErrMsg);
		}
	}
	sqlite3_close(db);
	return retVal;
}
/*********************************************************************/
//void sql_close(void)
//{
//	sqlite3_close(db);
//}

/*********************************************************************/
int SQLTask::init_sql(FILE *file)
{
	sqlite3 *db;
	std::string str;
	char *sql;
	char *zErrMsg = 0;
	const char *data = "sql_callback call";
	int retVal;

	/** 1. Open DB **/
	retVal = sqlite3_open("sloanNet.db",&db);
	if(retVal){
		//retVal != 0
		fprintf(file,"init_sql():Can't open database: %s\n :",sqlite3_errmsg(db));
		//SerialTask::printTimeStamp(file);
		sqlite3_close(db);
		exit(EXIT_FAILURE);
	}else{
		//fprintf(file,"sqlInit():Opened DB\n");
		/****************/
		/** 2. Create table **/
		str = "create table if not exists lwmTable (" \
			  " nodeType  int ," \
			  " productId sqlite_uint64 ,"
			  " shortAddr sqlite_uint16 primary key not null," \
			  " groupAddr sqlite_uint16, " \
			  " version   sqlite_uint16," \
			  " panId  sqlite_uint16," \
			  " channel int,"\
			  " lqi    int," \
			  " rssi   int," \
			  " status int not null," \
			  " maxVal  int not null," \
			  " minVal  int not null," \
			  " voltage  int not null," \
			  " current  int not null," \
			  " temperature  int not null," \
			  " dim  int not null);" ;
		sql = &str[0];
		retVal = sqlite3_exec(db,sql,sql_callback,(void *)data,&zErrMsg);
		if(retVal != SQLITE_OK ){
			//retVal != 0
			fprintf(file,"init_sql():Create-Error:%s \n",zErrMsg);
			sqlite3_free(zErrMsg);
			sqlite3_close(db);
			exit(EXIT_FAILURE);
		}
		//fprintf(file,"sqlInt():Create Table\n");
		sqlite3_close(db);
	}
	return retVal;
}

int SQLTask::update_sql(FILE *file,AppMessage_t *aappMsg)
{
	sqlite3 *db;
	std::string str;
	char *sql;
	char *zErrMsg = 0;
	const char *data = "sql_callback call";
	int retVal;


	retVal = sqlite3_open("sloanNet.db",&db);
	if(retVal){
		//retVal != 0
		fprintf(file,"update_sql():Can't open database: %s:\n",sqlite3_errmsg(db));

	}else{
		//retVal == 0
		/** 3. Insert/replace sql statement **/
		//printf("update_sql():update to sloaNetwork.db\n");
		str = sqlite3_mprintf("insert or replace into lwmTable ("
				"nodeType, productId, shortAddr,"
				"groupAddr, version,"
				"panId, channel, lqi, rssi,"
				"status, maxVal, minVal, "
				"voltage,current,temperature,"
				"dim ) values("
				"%u, %llx, %x,"
				"%x, %x,"
				"%u, %u, %u, %i,"   /*"(select shortAddr from lwm_table where shortAddr = %u)"*/
				"%u, %u, %u,"
				"%u, %u, %u, %u );",
				aappMsg->nodeType, aappMsg->productId, aappMsg->shortAddr,
				aappMsg->groupAddr, aappMsg->softVersion,
				aappMsg->panId, aappMsg->myChannel,	aappMsg->lqi, aappMsg->rssi,
				aappMsg->config.status, aappMsg->config.maxVal, aappMsg->config.minVal,
				aappMsg->data.voltage, aappMsg->data.current, aappMsg->data.temperature,
				aappMsg->data.dim );
		sql = &str[0];
		retVal = sqlite3_exec(db,sql,sql_callback,(void *)data,&zErrMsg);
		if(retVal != SQLITE_OK ){
			fprintf(file,"update_sql():Insert-Error:%s\n",zErrMsg);
			sqlite3_free(zErrMsg);
		}

	}
	sqlite3_close(db);
	return retVal;
}

int SQLTask::select_sql(FILE *file)
{
	sqlite3 *db;
	std::string str;
	char *sql;
	char *zErrMsg = 0;
	const char *data = "sql_callback call";
	int retVal;


	retVal = sqlite3_open("sloanNet.db",&db);
	if(retVal){
		//retVal != 0
		fprintf(file,"select_sql():Can't open database: %s:\n",sqlite3_errmsg(db));

	}else{
		//retVal == 0
		//printf("select_sql():select from sloaNetwork.db\n");
		str = "select * from lwmTable;";
		sql = &str[0];
		retVal = sqlite3_exec(db,sql,sql_callback,(void *)data,&zErrMsg);
		if(retVal != SQLITE_OK ){
			fprintf(file,"select_sql():select-Error:%s\n",zErrMsg);
			sqlite3_free(zErrMsg);
		}
	}
	sqlite3_close(db);
	return retVal;
}
