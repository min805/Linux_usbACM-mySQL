/*
 * SQLTask.h
 *
 *  Created on: Mar 15, 2016
 *      Author: min
 */

#ifndef SQLTASK_H_
#define SQLTASK_H_

#include "SerialTask.h"
#include </usr/sqlite3/include/sqlite3.h>

class SQLTask {
public:
	SQLTask();
	virtual ~SQLTask();

	int reset_lqi(FILE *file);
	int init_sql(FILE *file);
	int update_sql(FILE *file,AppMessage_t *appMsg);
	int select_sql(FILE *file);

private:



};

#endif /* SQLTASK_H_ */
