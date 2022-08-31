// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <windows.h>
#include <stdio.h>
#include <esent.h>

//  CallJ and Call both expect a local variable err, and a lable HandleError to be defined
//
#define CallJ( fn, label )  {                                           \
                            if ( ( err = (fn) ) < 0 )                       \
                                {                                   \
                                goto label;                         \
                                }                                   \
                            }
#define Call( fn )          CallJ( fn, HandleError )
#define NAME_SIZE 100
#define JET_BUFFER_SIZE 32000

typedef	struct _COLUMNLIST {
	int type;
	int id;
	char name[NAME_SIZE];
	char tableName[NAME_SIZE];
	struct _COLUMNLIST* next;
}COLUMNLIST;

typedef struct _JETColumnDefItem {
	JET_COLUMNDEF* jetColumn;
	struct _JETColumnDefItem* next;
} JETColumnDefElement;

void readColumnValue(JET_SESID sesid, JET_TABLEID tableid, COLUMNLIST* columnList) {
	unsigned char jetBuffer[JET_BUFFER_SIZE];
	unsigned long jetSize;
	JET_ERR err;

	//NOTE that this approach implies post processing multi valued columns if you re-use this code...
	JetRetrieveColumn(sesid, tableid, columnList->id, 0, 0, &jetSize, 0, 0);

#ifdef _DEBUG 
	//positive are warnings, -1047 is invalid buffer size which is expected here
	if (err < 0 && err != -1047) {
		printf("JetRetrieveColumn error : %i, jetSize : %d\n", err, jetSize);
		return(-2);
	}

	if (jetSize > JET_BUFFER_SIZE) {
		printf("Jet Buffer incorrect size preset: %d bytes are needed\n", jetSize);
		return(-2);
	}
#endif

	memset(jetBuffer, 0, JET_BUFFER_SIZE);

	switch (columnList->type) {
	case JET_coltypLong: // signed int types
		JetRetrieveColumn(sesid, tableid, columnList->id, jetBuffer, jetSize, 0, 0, 0);
		printf("%d\n", *(int*)jetBuffer);

		/*
		fprintf(dump,"%u_",*(unsigned int *)jetBuffer);
		for(unsigned int i=0;i<jetSize;i++)
		fprintf(dump,"%.2X",jetBuffer[i]);
		*/
		break;

	case JET_coltypCurrency: //signed long long type
		JetRetrieveColumn(sesid, tableid, columnList->id, jetBuffer, jetSize, 0, 0, 0);
		printf("%lld\n", *(long long*)jetBuffer);
		break;

	case JET_coltypBinary: // Raw binary types
		JetRetrieveColumn(sesid, tableid, columnList->id, jetBuffer, jetSize, 0, 0, 0);
		for (unsigned long i = 0; i < jetSize; i++)
			printf("%.2X", jetBuffer[i]);
		printf("\n");
		break;

	case JET_coltypLongBinary:
		/* We check matches on security descriptor, then SID
		*  to display accordingly, otherwise hex dump
		*/
		JetRetrieveColumn(sesid, tableid, columnList->id, jetBuffer, jetSize, 0, 0, 0);

		//hex dump
		for (unsigned long i = 0; i < jetSize; i++)
			printf("%.2X", jetBuffer[i]);
		printf("\n");
		break;
		// widechar text types
	case JET_coltypText:
	case JET_coltypLongText:
		JetRetrieveColumn(sesid, tableid, columnList->id, jetBuffer, jetSize, 0, 0, 0);
		printf("%s\n", jetBuffer);
		break;
	};
}


//JETColumnDefElement* loadColumns(JET_SESID sesid, JET_DBID dbid, char* tableName, char* columns[], int columnCnt) {
//	JET_ERR err;
//	JET_TABLEID tableid = JET_tableidNil;
//
//	JETColumnDefElement* listResult = (JETColumnDefElement*)malloc(sizeof(JETColumnDefElement));
//	JETColumnDefElement* tail = listResult;
//
//	Call(JetOpenTable(sesid, dbid, tableName, 0, 0, JET_bitTableReadOnly, &tableid));
//
//	for (int i = 0; i < columnCnt; i++) 
//	{
//		JET_COLUMNDEF* columndefid = malloc(sizeof(JET_COLUMNDEF));
//		Call(JetGetColumnInfo(sesid, dbid, tableName, columns[i], columndefid, sizeof(JET_COLUMNDEF), JET_ColInfo));
//
//		tail->next = (JETColumnDefElement*)malloc(sizeof(JETColumnDefElement));
//		tail = tail->next;
//		tail->jetColumn = columndefid;
//	}
//
//	return listResult;
//
//HandleError:
//	if (JET_tableidNil != tableid)
//	{
//		JetCloseTable(sesid, tableid);
//		tableid = JET_tableidNil;
//	}
//	return -1;
//}

int main(int argc, char* argv[]) {

	//Linked list to contain the datatable columns metadata
	COLUMNLIST* columnList, * msysObjectsColumns = (COLUMNLIST*)malloc(sizeof(COLUMNLIST));
	COLUMNLIST* listHead = (COLUMNLIST*)malloc(sizeof(COLUMNLIST));


	JET_ERR err;
	JET_INSTANCE instance = JET_instanceNil;
	JET_SESID sesid = JET_tableidNil;
	JET_DBID dbid = JET_tableidNil;
	JET_TABLEID tableid = JET_tableidNil;
	JET_TABLEID msgColumns = JET_tableidNil;

	/*
	JET_COLUMNDEF *columndefid = malloc(sizeof(JET_COLUMNDEF));
	JET_COLUMNDEF *columndeftype = malloc(sizeof(JET_COLUMNDEF));
	JET_COLUMNDEF *columndeftypecol = malloc(sizeof(JET_COLUMNDEF));
	JET_COLUMNDEF *columndefname = malloc(sizeof(JET_COLUMNDEF));
	JET_COLUMNDEF *columndefobjid = malloc(sizeof(JET_COLUMNDEF));
	*/

	JET_COLUMNDEF _columndefid;
	JET_COLUMNDEF _columndeftype;
	JET_COLUMNDEF _columndeftypecol;
	JET_COLUMNDEF _columndefname;
	JET_COLUMNDEF _columndefobjid;
	JET_COLUMNLIST _columnList;
	JET_COLUMNDEF _columndefattdata;

	JET_COLUMNDEF* columndefid = &_columndefid;
	JET_COLUMNDEF* columndeftype = &_columndeftype;
	JET_COLUMNDEF* columndeftypecol = &_columndeftypecol;
	JET_COLUMNDEF* columndefname = &_columndefname;
	JET_COLUMNDEF* columndefobjid = &_columndefobjid;
	JET_COLUMNLIST* colList = &_columnList;
	JET_COLUMNDEF* testcolumndef = &_columndefattdata;

	unsigned long a, b, c, d, e, att_size;
	long bufferid[16];
	char buffertype[256];
	char buffertypecol[8];
	char buffername[NAME_SIZE];
	char currentTable[NAME_SIZE];
	long bufferobjid[8];

	//Actually max buffer size should depend on the page size but it doesn't. Further investigation required.
	unsigned char jetBuffer[JET_BUFFER_SIZE];
	unsigned long jetSize;

	char* edbFilePath = argv[1];
	char* targetTable;
	char* tableName;
	// unsigned int datatableId = 0xffffffff;
	unsigned int i;

	FILE* dump = NULL;
	char* dumpFileName = "edb-dump.csv";
	//SYSTEMTIME lt;

	// RPC_WSTR Guid = NULL;
	// LPWSTR stringSid = NULL;
	long long sd_id = 0;
	unsigned long dbPageSize = 32768; // 0;
	unsigned int recordId = 0;

	listHead->next = NULL;
	columnList = listHead;

	//Our result file, don't modify if you want to use auto import scripts from dbbrowser
	//GetLocalTime(&lt);
	//sprintf_s(dumpFileName, 64, "%s_ntds_%02d-%02d-%04d_%02dh%02d.csv",argv[1], lt.wDay, lt.wMonth, lt.wYear, lt.wHour, lt.wMinute);
	// sprintf_s(dumpFileName, 64, "%s-ntds.dit-dump.csv", "edb");
	// fopen_s(&dump, dumpFileName, "w");

	// Initialize ESENT. 
	// See http://msdn.microsoft.com/en-us/library/windows/desktop/gg269297(v=exchg.10).aspx for error codes

	err = JetGetDatabaseFileInfo(edbFilePath, &dbPageSize, 4, JET_DbInfoPageSize);

	Call(JetSetSystemParameter(0, JET_sesidNil, JET_paramDatabasePageSize, dbPageSize, NULL));
	Call(JetSetSystemParameter(0, JET_sesidNil, JET_paramEnablePersistedCallbacks, 0, NULL));

	Call(JetCreateInstance(&instance, "edb-instance"));
	Call(JetSetSystemParameter(&instance, JET_sesidNil, JET_paramRecovery, (JET_API_PTR)L"Off", NULL));
	Call(JetSetSystemParameter(&instance, JET_sesidNil, JET_paramTempPath, 0, argv[2]));

	Call(JetInit(&instance));
	Call(JetBeginSession(instance, &sesid, 0, 0));
	Call(JetAttachDatabase(sesid, edbFilePath, JET_bitDbReadOnly));

	Call(JetOpenDatabase(sesid, edbFilePath, 0, &dbid, 0));

	//Let's enumerate the metadata about datatable (AD table)

	// tableName = "MSysObjects";
	tableName = "Msg";

	Call(JetOpenTable(sesid, dbid, tableName, 0, 0, JET_bitTableReadOnly, &tableid));

	printf("[*]Opened table: %s\n", tableName);

	err = JetGetColumnInfo(sesid, dbid, tableName, 0, colList, sizeof(JET_COLUMNLIST), JET_ColInfoList);

	err = JetMove(sesid, colList->tableid, JET_MoveFirst, 0);

	do {
		Call(JetRetrieveColumn(sesid, colList->tableid, colList->columnidcolumnid, 0, 0, &a, 0, 0));
		Call(JetRetrieveColumn(sesid, colList->tableid, colList->columnidcolumnid, bufferid, a, 0, 0, 0));

		Call(JetRetrieveColumn(sesid, colList->tableid, colList->columnidcolumnname, 0, 0, &c, 0, 0));
		Call(JetRetrieveColumn(sesid, colList->tableid, colList->columnidcolumnname, buffername, c, 0, 0, 0));
		buffername[c] = '\0';

		printf(" - %s\n", buffername);
	} while (JetMove(sesid, colList->tableid, JET_MoveNext, 0) == JET_errSuccess);

	//Obtain structures with necessary information to retrieve column values

	// Call(JetGetColumnInfo(sesid, dbid, tableName, "N3701", columndefattdata, sizeof(JET_COLUMNDEF), JET_ColInfo));
	Call(JetGetColumnInfo(sesid, dbid, tableName, "N67b0", testcolumndef, sizeof(JET_COLUMNDEF), JET_ColInfo));

	//Call( JetGetColumnInfo(sesid, dbid, tableName, "Id", columndefid, 100 /*sizeof(JET_COLUMNDEF)*/, JET_ColInfo));
	//Call( JetGetColumnInfo(sesid, dbid, tableName, "Type", columndeftype, sizeof(JET_COLUMNDEF), JET_ColInfo) );
	//Call( JetGetColumnInfo(sesid, dbid, tableName, "ColtypOrPgnoFDP", columndeftypecol, sizeof(JET_COLUMNDEF), JET_ColInfo) );
	//Call( JetGetColumnInfo(sesid, dbid, tableName, "Name", columndefname, sizeof(JET_COLUMNDEF), JET_ColInfo) );
	//Call( JetGetColumnInfo(sesid, dbid, tableName, "ObjIdTable", columndefobjid, sizeof(JET_COLUMNDEF), JET_ColInfo) );

	//Position the cursor at the first record
	Call(JetMove(sesid, tableid, JET_MoveFirst, 0));

	//Retrieve columns metadata	
	do
	{
		JET_ERR err = JetRetrieveColumn(sesid, tableid, testcolumndef->columnid, 0, 0, &att_size, 0, 0);
		char* att_data_buffer = malloc(att_size);

		err = JetRetrieveColumn(sesid, tableid, testcolumndef->columnid, att_data_buffer, att_size, 0, 0, 0);

		printf("%8d MsgId: ", ++recordId);
		for (unsigned int i = 0; i < att_size; i++) {
			printf("%02x", att_data_buffer[i]);
		}
		printf("\n");
		free(att_data_buffer);

		// Call( JetRetrieveColumn(sesid, tableid, columndefid->columnid, 0, 0, &a, 0, 0) );
		// Call( JetRetrieveColumn(sesid, tableid, columndefid->columnid, bufferid, a, 0, 0, 0) );

		// Call( JetRetrieveColumn(sesid, tableid, columndeftype->columnid, 0, 0, &b, 0, 0) );
		// Call( JetRetrieveColumn(sesid, tableid, columndeftype->columnid, buffertype, b, 0, 0, 0));

		// Call( JetRetrieveColumn(sesid, tableid, columndeftypecol->columnid, 0, 0, &e, 0, 0));
		// Call( JetRetrieveColumn(sesid, tableid, columndeftypecol->columnid, buffertypecol, e, 0, 0, 0));

		// Call( JetRetrieveColumn(sesid, tableid, columndefname->columnid, 0, 0, &c, 0, 0));
		// Call( JetRetrieveColumn(sesid, tableid, columndefname->columnid, buffername, c, 0, 0, 0));
		// buffername[c] = '\0';

		// Call( JetRetrieveColumn(sesid, tableid, columndefobjid->columnid, 0, 0, &d, 0, 0));
		// Call( JetRetrieveColumn(sesid, tableid, columndefobjid->columnid, bufferobjid, d, 0, 0, 0));

		//We got the correct type and table id, let's dump the column name and add it to the column list
		//if (buffertype[0] == (char)1) 
		//{
		//	strcpy(currentTable, buffername);
		//}
		//else if (buffertype[0] == (char)2)
		//{
		//	unsigned int j;
		//	columnList->next = (COLUMNLIST*)malloc(sizeof(COLUMNLIST));
		//	if (!columnList->next) {
		//		printf("Memory allocation failed during metadata dump\n");
		//		return(-1);
		//	}
		//	columnList = columnList->next;
		//	columnList->next = NULL;

		//	strcpy(columnList->name, buffername);
		//	strcpy(columnList->tableName, currentTable);

		//	/*for (j = 0; j < c; j++)
		//		columnList->name[j] = buffername[j];
		//	columnList->name[c] = '\0';*/
		//	columnList->type = buffertypecol[0];
		//	columnList->id = bufferid[0];
		//}
	} while (JetMove(sesid, tableid, JET_MoveNext, 0) == JET_errSuccess);

	Call(JetCloseTable(sesid, tableid));


	//Let's use our metadata to dump the whole AD schema
	tableName = "MSysObjects";
	Call(JetOpenTable(sesid, dbid, tableName, 0, 0, JET_bitTableReadOnly, &tableid));

	printf("[*]Opened table: %s\n", tableName);
	printf("Dumping %s column names...\n", tableName);

	columnList = listHead;
	while (columnList->next)
	{
		columnList = columnList->next;
		printf("Column: %s / %s\n", columnList->tableName, columnList->name);
	}

	printf("Dumping content...\n");

	Call(JetMove(sesid, tableid, JET_MoveFirst, 0));
	do
	{
		columnList = listHead;
		while (columnList->next)
		{
			columnList = columnList->next;
			// printf("Column: %s / %s\n", columnList->tableName, columnList->name);
			readColumnValue(sesid, tableid, columnList);
		}
	} while (JetMove(sesid, tableid, JET_MoveNext, 0) == JET_errSuccess);

HandleError:
	if (err < 0)
	{
		printf("Sample failed with error %d.\n", err);
	}
	else
	{
		printf("Sample succeeded.\n");
	}

	////////////////////////////////////////////////////////
	//  TERMINATION
	//
	if (JET_tableidNil != tableid)
	{
		JetCloseTable(sesid, tableid);
		tableid = JET_tableidNil;
	}

	if (JET_dbidNil != dbid)
	{
		JetCloseDatabase(sesid, dbid, 0);
		dbid = JET_dbidNil;
	}

	if (JET_sesidNil != sesid)
	{
		JetEndSession(sesid, 0);
		sesid = JET_sesidNil;
	}

	//  always call JetTerm!!!
	//
	if (0 /* JET_instanceNil */ != instance)
	{
		JetTerm(instance);
		instance = 0;
	}

	return 0;
}