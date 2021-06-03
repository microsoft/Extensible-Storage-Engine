;// Copyright (c) Microsoft Corporation.
;// Licensed under the MIT License.

;////////////////////////////////////////////////////////////////////////////////////////
;//	
;//	Event IDs
;//
;// When adding an Event Message, please ensure that you're adding it to the right 
;// section of the file.
;//
;////////////////////////////////////////////////////////////////////////////////////////

LanguageNames=(English=0x409:jetmsg)
MessageIdTypedef=MessageId

MessageId=0
SymbolicName=PLAIN_TEXT_ID
Language=English
%1 (%2) %3%4
.

;///////////////////////////////////////////////////////////
;//	Categories messages
;///////////////////////////////////////////////////////////

MessageId=1
SymbolicName=GENERAL_CATEGORY
Language=English
General
.

MessageId=2
SymbolicName=BUFFER_MANAGER_CATEGORY
Language=English
Database Page Cache
.

MessageId=3
SymbolicName=LOGGING_RECOVERY_CATEGORY
Language=English
Logging/Recovery
.

MessageId=4
SymbolicName=SPACE_MANAGER_CATEGORY
Language=English
Space Management
.

MessageId=5
SymbolicName=DATA_DEFINITION_CATEGORY
Language=English
Table/Column/Index Definition
.

MessageId=6
SymbolicName=DATA_MANIPULATION_CATEGORY
Language=English
Record Manipulation
.

MessageId=7
SymbolicName=PERFORMANCE_CATEGORY
Language=English
Performance
.

MessageId=8
SymbolicName=REPAIR_CATEGORY
Language=English
Database Repair
.

MessageId=9
SymbolicName=CONVERSION_CATEGORY
Language=English
Database Conversion
.

MessageId=10
SymbolicName=ONLINE_DEFRAG_CATEGORY
Language=English
Online Defragmentation
.

MessageId=11
SymbolicName=SYSTEM_PARAMETER_CATEGORY
Language=English
System Parameter Settings
.

MessageId=12
SymbolicName=DATABASE_CORRUPTION_CATEGORY
Language=English
Database Corruption
.

MessageId=13
SymbolicName=DATABASE_ZEROING_CATEGORY
Language=English
Database Zeroing
.

MessageId=14
SymbolicName=TRANSACTION_MANAGER_CATEGORY
Language=English
Transaction Manager
.

MessageId=15
SymbolicName=RFS2_CATEGORY
Language=English
Resource Failure Simulation
.

MessageId=16
SymbolicName=OS_SNAPSHOT_BACKUP
Language=English
ShadowCopy
.

MessageId=17
SymbolicName=FAILURE_ITEM_CATEGORY
Language=English
Failure Items
.

MessageId=18
SymbolicName=DEVICE_CATEGORY
Language=English
System Device
.

MessageId=19
SymbolicName=BLOCK_CACHE_CATEGORY
Language=English
System Device
.

MessageId=20
SymbolicName=MAC_CATEGORY
Language=English
<EOL>
.

;// You are almost assuredly not adding in the right place? Add to categories above MAC_CATEGORY and increment MAC_CATEGORY.


;///////////////////////////////////////////////////////////
;//	System / Instance start and stop messages
;///////////////////////////////////////////////////////////

MessageId=100
SymbolicName=START_ID
Language=English
%1 (%2) %3The database engine %4.%5.%6.%7 started.
.

MessageId=101
SymbolicName=STOP_ID
Language=English
%1 (%2) %3The database engine stopped.
.

MessageId=102
SymbolicName=START_INSTANCE_ID
Language=English
%1 (%2) %3The database engine (%5.%6.%7.%8) is starting a new instance (%4).
.

MessageId=103
SymbolicName=STOP_INSTANCE_ID
Language=English
%1 (%2) %3The database engine stopped the instance (%4).
%n
%nDirty Shutdown: %6
%n
%nInternal Timing Sequence: %5
.

MessageId=104
SymbolicName=STOP_INSTANCE_ID_WITH_ERROR
Language=English
%1 (%2) %3The database engine stopped the instance (%4) with error (%5).
%n
%nInternal Timing Sequence: %6
.

MessageId=105
SymbolicName=START_INSTANCE_DONE_ID
Language=English
%1 (%2) %3The database engine started a new instance (%4). (Time=%5 seconds)
%n
%nAdditional Data:%n
%7
%n
%nInternal Timing Sequence: %6
.

MessageId=106
SymbolicName=CONFIG_STORE_PARAM_OVERRIDE_ID
Language=English
%1 (%2) %3The parameter %4 was attempted to be set to %5, but was overridden to %6 by the registry settings (at %7).
.

MessageId=107
SymbolicName=CONFIG_STORE_PARAM_DEFAULT_LOAD_FAIL_ID
Language=English
%1 (%2) %3The parameter %4 was read from the registry settings (at %7), but the ESE engine rejected the value %5 with err %6.
.

MessageId=108
SymbolicName=CONFIG_STORE_READ_INHIBIT_PAUSE_ID
Language=English
%1 (%2) %3The specific ESE configuration store is locked in a read inhibit state, clear the %1 registry value to enable ESE to continue and utilize the config store.
.

;// You are almost assuredly not adding in the right place?



;///////////////////////////////////////////////////////////
;//	Backup Events
;///////////////////////////////////////////////////////////

MessageId=200
SymbolicName=START_FULL_BACKUP_ID
Language=English
%1 (%2) %3The database engine is starting a full backup.
.

MessageId=201
SymbolicName=START_INCREMENTAL_BACKUP_ID
Language=English
%1 (%2) %3The database engine is starting an incremental backup.
.

MessageId=202
SymbolicName=STOP_BACKUP_ID
Language=English
%1 (%2) %3The database engine has completed the backup procedure successfully.
.

MessageId=203
SymbolicName=STOP_BACKUP_ERROR_ID
Language=English
%1 (%2) %3The database engine has stopped the backup with error %4.
.

MessageId=204
SymbolicName=START_RESTORE_ID
Language=English
%1 (%2) %3The database engine is restoring from backup. Restore will begin replaying logfiles in folder %4 and continue rolling forward logfiles in folder %5.
.

MessageId=205
SymbolicName=STOP_RESTORE_ID
Language=English
%1 (%2) %3The database engine has stopped restoring.
.

MessageId=206
SymbolicName=DATABASE_MISS_FULL_BACKUP_ERROR_ID
Language=English
%1 (%2) %3Database %4 cannot be incrementally backed-up. You must first perform a full backup before performing an incremental backup.
.

MessageId=207
SymbolicName=STOP_BACKUP_ERROR_ABORT_BY_CALLER
Language=English
%1 (%2) %3The database engine has stopped backup because it was halted by the client or the connection with the client failed.
.

MessageId=210
SymbolicName=START_FULL_BACKUP_INSTANCE_ID
Language=English
%1 (%2) %3A full backup is starting.
.

MessageId=211
SymbolicName=START_INCREMENTAL_BACKUP_INSTANCE_ID
Language=English
%1 (%2) %3An incremental backup is starting.
.

MessageId=212
SymbolicName=START_SNAPSHOT_BACKUP_INSTANCE_ID
Language=English
%1 (%2) %3A shadow copy backup is starting.
.

MessageId=213
SymbolicName=STOP_BACKUP_INSTANCE_ID
Language=English
%1 (%2) %3The backup procedure has been successfully completed.
.

MessageId=214
SymbolicName=STOP_BACKUP_ERROR_INSTANCE_ID
Language=English
%1 (%2) %3The backup has stopped with error %4.
.

MessageId=215
SymbolicName=STOP_BACKUP_ERROR_ABORT_BY_CALLER_INSTANCE_ID
Language=English
%1 (%2) %3The backup has been stopped because it was halted by the client or the connection with the client failed.
.

MessageId=216
SymbolicName=DB_LOCATION_CHANGE_DETECTED
Language=English
%1 (%2) %3A database location change was detected from '%4' to '%5'.
.

MessageId=217
SymbolicName=BACKUP_ERROR_FOR_ONE_DATABASE
Language=English
%1 (%2) %3Error (%4) during backup of a database (file %5). The database will be unable to restore.
.

MessageId=218
SymbolicName=BACKUP_ERROR_READ_FILE
Language=English
%1 (%2) %3Error (%4) during copy or backup of file %5.
.

MessageId=219
SymbolicName=BACKUP_ERROR_INFO_UPDATE
Language=English
%1 (%2) %3Error (%4) occured during database headers update with the backup information.
.

MessageId=220
SymbolicName=BACKUP_FILE_START
Language=English
%1 (%2) %3Starting the copy or backup of the file %4 (size %5).
.

MessageId=221
SymbolicName=BACKUP_FILE_STOP_OK
Language=English
%1 (%2) %3Ending the copy or backup of the file %4.
.

MessageId=222
SymbolicName=BACKUP_FILE_STOP_BEFORE_END
Language=English
%1 (%2) %3Ending the backup of the file %4. Not all data in the file has been read (read %5 bytes out of %6 bytes).
.

MessageId=223
SymbolicName=BACKUP_LOG_FILES_START
Language=English
%1 (%2) %3Starting the backup of log files (range %4 - %5). 
.

MessageId=224
SymbolicName=BACKUP_TRUNCATE_LOG_FILES
Language=English
%1 (%2) %3Deleting log files %4 to %5. 
.

MessageId=225
SymbolicName=BACKUP_NO_TRUNCATE_LOG_FILES
Language=English
%1 (%2) %3No log files can be truncated. 
.

MessageId=226
SymbolicName=STOP_BACKUP_ERROR_ABORT_BY_SERVER_INSTANCE_ID
Language=English
%1 (%2) %3The backup has been stopped prematurely (possibly because the instance is terminating).
.

MessageId=227
SymbolicName=BACKUP_TRUNCATE_FOUND_MISSING_LOG_FILES
Language=English
%1 (%2) %3There were %4 log file(s) not found in the log range we attempted to truncate. 
.

;// !!! ARE YOU SURE you're adding this in the right place !!! ???



;///////////////////////////////////////////////////////////
;//	Internal Copy is similar to backup.
;///////////////////////////////////////////////////////////

MessageId=240
SymbolicName=START_INTERNAL_COPY_INSTANCE_ID
Language=English
%1 (%2) %3An internal copy (for seeding or analysis purposes) is starting. The streaming ESE backup APIs are being used for the transfer method.
.

MessageId=243
SymbolicName=STOP_INTERNAL_COPY_INSTANCE_ID
Language=English
%1 (%2) %3The internal database copy (for seeding or analysis purposes) procedure has been successfully completed.
.

MessageId=244
SymbolicName=STOP_INTERNAL_COPY_ERROR_INSTANCE_ID
Language=English
%1 (%2) %3The internal database copy (for seeding or analysis purposes) has stopped. Error: %4.
.

MessageId=245
SymbolicName=STOP_INTERNAL_COPY_ERROR_ABORT_BY_CALLER_INSTANCE_ID
Language=English
%1 (%2) %3The internal database copy (for seeding or analysis purposes) has been stopped because it was halted by the client or because the connection with the client failed.
.

MessageId=252
SymbolicName=INTERNAL_COPY_FILE_STOP_BEFORE_END
Language=English
%1 (%2) %3Ending the internal copy (for seeding or analysis purposes) of the file %4. Not all data in the file has been read (read %5 bytes out of %6 bytes).
.

MessageId=256
SymbolicName=STOP_INTERNAL_COPY_ERROR_ABORT_BY_SERVER_INSTANCE_ID
Language=English
%1 (%2) %3The internal database copy (for seeding or analysis purposes) has been stopped prematurely, possibly because the instance is terminating.
.

MessageId=257
SymbolicName=SUSPEND_INTERNAL_COPY_DURING_RECOVERY_INSTANCE_ID
Language=English
%1 (%2) %3The internal database copy (for seeding or analysis purposes) of database %4 during recovery has been suspended. It took %5 seconds to wait for client to close the backup context.
.

;// !!! ARE YOU SURE you're adding this in the right place !!! ???



;///////////////////////////////////////////////////////////
;//	Instance Recovery / Start status
;///////////////////////////////////////////////////////////

MessageId=300
SymbolicName=START_REDO_ID
Language=English
%1 (%2) %3The database engine is initiating recovery steps.
.

MessageId=301
SymbolicName=STATUS_REDO_ID
Language=English
%1 (%2) %3The database engine has finished replaying logfile %4.
%n
%nProcessing Stats: %5
%nLog record of type '%6' was seen most frequently (%7 times)
.

MessageId=302
SymbolicName=STOP_REDO_ID
Language=English
%1 (%2) %3The database engine has successfully completed recovery steps.
.

; // This error message is too generic, no one should use it.
MessageId=303
SymbolicName=ERROR_ID_DEPRECATED
Language=English
%1 (%2) %3Database engine error %4 occurred.
.

MessageId=304
SymbolicName=STOP_REDO_WITHOUT_UNDO_ID
Language=English
%1 (%2) %3The database engine has successfully completed replay steps.
.

;// You are almost assuredly not adding in the right place?


;// !!! ARE YOU SURE you're adding this in the right place !!! ???


;///////////////////////////////////////////////////////////
;//	Database Create / Attach / Detach status
;///////////////////////////////////////////////////////////

MessageId=325
SymbolicName=CREATE_DATABASE_DONE_ID
Language=English
%1 (%2) %3The database engine created a new database (%4, %5). (Time=%6 seconds)
%n
%nAdditional Data: %9
%n
%nInternal Timing Sequence: %7
.

MessageId=326
SymbolicName=ATTACH_DATABASE_DONE_ID
Language=English
%1 (%2) %3The database engine attached a database (%4, %5). (Time=%6 seconds)
%n
%nSaved Cache: %8
%nAdditional Data: %9
%n
%nInternal Timing Sequence: %7
.

MessageId=327
SymbolicName=DETACH_DATABASE_DONE_ID
Language=English
%1 (%2) %3The database engine detached a database (%4, %5). (Time=%6 seconds)
%n
%nRevived Cache: %8
%nAdditional Data: %9
%n
%nInternal Timing Sequence: %7
.

MessageId=328
SymbolicName=RECOVERY_DB_ATTACH_ID
Language=English
%1 (%2) %3The database engine has fired callback for attach of database %4 during recovery at log position %5. The callback returned %6.
.

MessageId=329
SymbolicName=RECOVERY_DB_DETACH_ID
Language=English
%1 (%2) %3The database engine has fired callback for detach of database %4 during recovery at log position %5. The callback returned %6.
.

MessageId=330
SymbolicName=ATTACH_DATABASE_VERSION_UPGRADE_SKIPPED_ID
Language=English
%1 (%2) %3The database [%4] format version is being held back to %6 due to application parameter setting of %5. Current default engine version: %7.
.

MessageId=331
SymbolicName=ATTACH_DATABASE_VERSION_UPGRADED_ID
Language=English
%1 (%2) %3The database [%4] version was upgraded from %5 to %6. Current engine format version parameter setting: %7
.

MessageId=332
SymbolicName=ATTACH_DATABASE_VERSION_TOO_HIGH_FOR_ENGINE_ID
Language=English
%1 (%2) %3The database [%4] version %5 is higher than the maximum version understood by the engine %6.
.

MessageId=333
SymbolicName=ATTACH_DATABASE_VERSION_TOO_HIGH_FOR_PARAM_ID
Language=English
%1 (%2) %3The database [%4] version %5 is higher than the maximum version configured by the application %6. Current engine format version parameter setting: %7
.

MessageId=334
SymbolicName=DATABASE_INCREMENTAL_RESEED_PATCH_COMPLETE_ID
Language=English
%1 (%2) %3The database [%4] has completed incremental reseed page patching operations for %5 pages.
%n
%nDetails:
%nRange of Patching: %6
%nTiming: %7
.

MessageId=335
SymbolicName=REDO_DEFERRED_ATTACH_REASON_ID
Language=English
%1 (%2) %3Replay of a %4 for database "%5" at log position %6 was deferred due to %7.  Additional information:  %8
.

;// You are almost assuredly not adding in the right place?


;// !!! ARE YOU SURE you're adding this in the right place !!! ???



;///////////////////////////////////////////////////////////
;//	All Manner of Badness and Corruption
;///////////////////////////////////////////////////////////

;// at least all manner of badness seems to be what is here ...
;// Note: It's weird we used 398 and 399 ... and we go straight through 400 to the almost 1/3rd of the way
;// into the 500s ... some sections may have ended up merging, it looks like with the repair section judging
;// from the early 500s.  Some of these events are dead, such as S_O_READ_PAGE_TIME_ERROR_ID.

MessageId=398
SymbolicName=DATABASE_PAGE_CHECK_FAILED_ID
Language=English
%1 (%2) %3The database page read from the file "%4" at offset %5 (database page %9) for %6 bytes failed verification. Bit %8 was corrupted and was corrected but page verification failed with error %7.  This problem is likely due to faulty hardware and may continue. Transient failures such as these can be a precursor to a catastrophic failure in the storage subsystem containing this file.  Please contact your hardware vendor for further assistance diagnosing the problem.
.

MessageId=399
SymbolicName=DATABASE_PAGE_CHECKSUM_CORRECTED_ID
Language=English
%1 (%2) %3The database page read from the file "%4" at offset %5 (database page %8) for %6 bytes failed verification. Bit %7 was corrupted and has been corrected.  This problem is likely due to faulty hardware and may continue. Transient failures such as these can be a precursor to a catastrophic failure in the storage subsystem containing this file.  Please contact your hardware vendor for further assistance diagnosing the problem.
.

MessageId=400
SymbolicName=S_O_READ_PAGE_TIME_ERROR_ID
Language=English
%1 (%2) %3Synchronous overlapped read page time error %4 occurred. If this error persists, please restore the database from a previous backup.
.

MessageId=401
SymbolicName=S_O_WRITE_PAGE_ISSUE_ERROR_ID
Language=English
%1 (%2) %3Synchronous overlapped write page issue error %4 occurred.
.

MessageId=402
SymbolicName=S_O_WRITE_PAGE_ERROR_ID
Language=English
%1 (%2) %3Synchronous overlapped write page error %4 occurred.
.

MessageId=403
SymbolicName=S_O_PATCH_FILE_WRITE_PAGE_ERROR_ID
Language=English
%1 (%2) %3Synchronous overlapped patch file write page error %4 occurred.
.

MessageId=404
SymbolicName=S_READ_PAGE_TIME_ERROR_ID
Language=English
%1 (%2) %3Synchronous read page checksum error %4 occurred. If this error persists, please restore the database from a previous backup.
.

MessageId=405
SymbolicName=PRE_READ_PAGE_TIME_ERROR_ID
Language=English
%1 (%2) %3Pre-read page checksum error %4 occurred. If this error persists, please restore the database from a previous backup.
.

MessageId=406
SymbolicName=A_DIRECT_READ_PAGE_CORRUPTTED_ERROR_ID
Language=English
%1 (%2) %3Direct read found corrupted page %4 with error %5. If this error persists, please restore the database from a previous backup.
.

MessageId=407
SymbolicName=BFIO_TERM_ID
Language=English
%1 (%2) %3Buffer I/O thread termination error %4 occurred.
.

MessageId=408
SymbolicName=LOG_WRITE_ERROR_ID
Language=English
%1 (%2) %3Unable to write to logfile %4. Error %5.
.

MessageId=409
SymbolicName=LOG_HEADER_WRITE_ERROR_ID
Language=English
%1 (%2) %3Unable to write to the header of logfile %4. Error %5.
.

MessageId=410
SymbolicName=LOG_READ_ERROR_ID
Language=English
%1 (%2) %3Unable to read logfile %4. Error %5.
.

MessageId=411
SymbolicName=LOG_BAD_VERSION_ERROR_ID
Language=English
%1 (%2) %3The log version stamp of logfile %4 does not match the database engine version stamp. The logfiles may be the wrong version for the database.
.

MessageId=412
SymbolicName=LOG_HEADER_READ_ERROR_ID
Language=English
%1 (%2) %3Unable to read the header of logfile %4. Error %5.
.

MessageId=413
SymbolicName=NEW_LOG_ERROR_ID
Language=English
%1 (%2) %3Unable to create a new logfile because the database cannot write to the log drive. The drive may be read-only, out of disk space, misconfigured, or corrupted. Error %4.
.

MessageId=414
SymbolicName=LOG_FLUSH_WRITE_0_ERROR_ID
Language=English
%1 (%2) %3Unable to write to section 0 while flushing logfile %4. Error %5.
.

MessageId=415
SymbolicName=LOG_FLUSH_WRITE_1_ERROR_ID
Language=English
%1 (%2) %3Unable to write to section 1 while flushing logfile %4. Error %5.
.

MessageId=416
SymbolicName=LOG_FLUSH_WRITE_2_ERROR_ID
Language=English
%1 (%2) %3Unable to write to section 2 while flushing logfile %4. Error %5.
.

MessageId=417
SymbolicName=LOG_FLUSH_WRITE_3_ERROR_ID
Language=English
%1 (%2) %3Unable to write to section 3 while flushing logfile %4. Error %5.
.

MessageId=418
SymbolicName=LOG_FLUSH_OPEN_NEW_FILE_ERROR_ID
Language=English
%1 (%2) %3Error %5 occurred while opening a newly-created logfile %4.
.

MessageId=419
SymbolicName=RECOVERY_DATABASE_READ_PAGE_ERROR_ID
Language=English
%1 (%2) %3Unable to read page %5 of database %4.%n
Additional information:%n
%tError code: %6%n
%tLog position: %7%n
%tPage timestamp: %8%n
.

MessageId=420
SymbolicName=RESTORE_DATABASE_READ_HEADER_WARNING_ID
Language=English
%1 (%2) %3Unable to read the header of database %4. Error %5. The database may have been moved and therefore may no longer be located where the logs expect it to be.
.

MessageId=421
SymbolicName=RESTORE_DATABASE_PARTIALLY_ERROR_ID
Language=English
%1 (%2) %3The database %4 created at %5 was not recovered. The recovered database was created at %5.
.

MessageId=422
SymbolicName=RESTORE_DATABASE_MISSED_ERROR_ID
Language=English
%1 (%2) %3The database %4 created at %5 was not recovered.
.

MessageId=423
SymbolicName=BAD_PAGE
Language=English
%1 (%2) %3The database engine found a bad page.
.

MessageId=424
SymbolicName=DISK_FULL_ERROR_ID
Language=English
%1 (%2) %3The database disk is full. Deleting logfiles to recover disk space may make the database unstartable if the database file(s) are not in a Clean Shutdown state. Numbered logfiles may be moved, but not deleted, if and only if the database file(s) are in a Clean Shutdown state. Do not move %4.
.

MessageId=425
SymbolicName=LOG_DATABASE_MISMATCH_ERROR_ID
Language=English
%1 (%2) %3The database signature does not match the log signature for database %4.
.

MessageId=426
SymbolicName=FILE_NOT_FOUND_ERROR_ID
Language=English
%1 (%2) %3The database engine could not find the file or folder called %4.
.

MessageId=427
SymbolicName=FILE_ACCESS_DENIED_ERROR_ID
Language=English
%1 (%2) %3The database engine could not access the file called %4.
.

MessageId=428
SymbolicName=LOW_LOG_DISK_SPACE
Language=English
%1 (%2) %3The database engine is rejecting update operations due to low free disk space on the log disk.
.

MessageId=429
SymbolicName=LOG_DISK_FULL
Language=English
%1 (%2) %3The database engine log disk is full. Deleting logfiles to recover disk space may make your database unstartable if the database file(s) are not in a Clean Shutdown state. Numbered logfiles may be moved, but not deleted, if and only if the database file(s) are in a Clean Shutdown state. Do not move %4.
.

MessageId=430
SymbolicName=DATABASE_PATCH_FILE_MISMATCH_ERROR_ID
Language=English
%1 (%2) %3Database %4 and its patch file do not match.
.

MessageId=431
SymbolicName=STARTING_RESTORE_LOG_TOO_HIGH_ERROR_ID
Language=English
%1 (%2) %3The starting restored logfile %4 is too high. It should start from logfile %5.
.

MessageId=432
SymbolicName=ENDING_RESTORE_LOG_TOO_LOW_ERROR_ID
Language=English
%1 (%2) %3The ending restored logfile %4 is too low. It should end at logfile %5.
.

MessageId=433
SymbolicName=RESTORE_LOG_FILE_HAS_BAD_SIGNATURE_ERROR_ID
Language=English
%1 (%2) %3The restored logfile %4 does not have the correct log signature.
.

MessageId=434
SymbolicName=RESTORE_LOG_FILE_NOT_CONTIGUOUS_ERROR_ID
Language=English
%1 (%2) %3The timestamp for restored logfile %4 does not match the timestamp recorded in the logfile previous to it.
.

MessageId=435
SymbolicName=RESTORE_LOG_FILE_MISSING_ERROR_ID
Language=English
%1 (%2) %3The restored logfile %4 is missing.
.

MessageId=436
SymbolicName=EXISTING_LOG_FILE_HAS_BAD_SIGNATURE_ERROR_ID
Language=English
%1 (%2) %3The signature of logfile %4 does not match other logfiles. Recovery cannot succeed unless all signatures match. Logfiles %5 to %6 have been deleted.
.

MessageId=437
SymbolicName=EXISTING_LOG_FILE_NOT_CONTIGUOUS_ERROR_ID
Language=English
%1 (%2) %3There is a gap in sequence number between logfile %4 and the logfiles previous to it. Logfiles %5 to 0x%6 have been deleted so that recovery can complete.
.

MessageId=438
SymbolicName=BAD_BACKUP_DATABASE_SIZE
Language=English
%1 (%2) %3The backup database %4 must be a multiple of 4 KB.
.

MessageId=439
SymbolicName=SHADOW_PAGE_WRITE_FAIL_ID
Language=English
%1 (%2) %3Unable to write a shadowed header for file %4. Error %5.
.

MessageId=440
SymbolicName=LOG_FILE_CORRUPTED_ID
Language=English
%1 (%2) %3The log file %4 is damaged, invalid, or inaccessible (error %5) and cannot be used. If this log file is required for recovery, a good copy of the log file will be needed for recovery to complete successfully.
.

MessageId=441
SymbolicName=DB_FILE_SYS_ERROR_ID
Language=English
%1 (%2) %3File system error %5 during IO on database %4. If this error persists, the database file may be damaged and may need to be restored from a previous backup.
.

MessageId=442
SymbolicName=DB_IO_SIZE_ERROR_ID
Language=English
%1 (%2) %3IO size mismatch on database %4, IO size %5 expected while returned size is %6.
.

MessageId=443
SymbolicName=LOG_FILE_SYS_ERROR_ID
Language=English
%1 (%2) %3File system error %5 during IO or flush on logfile %4.
.

MessageId=444
SymbolicName=LOG_IO_SIZE_ERROR_ID
Language=English
%1 (%2) %3IO size mismatch on logfile %4, IO size %5 expected while returned size is %6.
.

MessageId=445
SymbolicName=SPACE_MAX_DB_SIZE_REACHED_ID
Language=English
%1 (%2) %3The database %4 has reached its maximum size of %5 MB. If the database cannot be restarted, an offline defragmentation may be performed to reduce its size.
.

MessageId=446
SymbolicName=REDO_END_ABRUPTLY_ERROR_ID
Language=English
%1 (%2) %3Database recovery stopped abruptly while redoing logfile %4 (%5,%6). The logs after this point may not be recognizable and will not be processed.
.

MessageId=447
SymbolicName=BAD_PAGE_LINKS_ID
Language=English
%1 (%2) %3A bad page link (error %4) has been detected in a B-Tree (ObjectId: %5, PgnoRoot: %6) of database %7 (%8 => %9, %10).%n
Tag: %11%n
Fatal: %12%n
.

MessageId=448
SymbolicName=CORRUPT_LONG_VALUE_ID
Language=English
%1 (%2) %3Data inconsistency detected in table %4 of database %5 (%6,%7).
.

MessageId=449
SymbolicName=CORRUPT_SLV_SPACE_ID
Language=English
%1 (%2) %3Streaming data allocation inconsistency detected (%4,%5).
.

MessageId=450
SymbolicName=CURRENT_LOG_FILE_MISSING_ERROR_ID
Language=English
%1 (%2) %3A gap in the logfile sequence was detected. Logfile %4 is missing.  Other logfiles past this one may also be required. This message may appear again if the missing logfiles are not restored.
.

MessageId=451
SymbolicName=LOG_FLUSH_WRITE_4_ERROR_ID
Language=English
%1 (%2) %3Unable to write to section 4 while flushing logfile %4. Error %5.
.

MessageId=452
SymbolicName=REDO_MISSING_LOW_LOG_ERROR_ID
Language=English
%1 (%2) %3Database %4 requires logfiles %5-%6 in order to recover successfully. Recovery could only locate logfiles starting at %7.
.

MessageId=453
SymbolicName=REDO_MISSING_HIGH_LOG_ERROR_ID
Language=English
%1 (%2) %3Database %4 requires logfiles %5-%6 (%8 - %9) in order to recover successfully. Recovery could only locate logfiles up to %7 (%10).
.

MessageId=454
SymbolicName=RESTORE_DATABASE_FAIL_ID
Language=English
%1 (%2) %3Database recovery/restore failed with unexpected error %4.
.

MessageId=455
SymbolicName=LOG_OPEN_FILE_ERROR_ID
Language=English
%1 (%2) %3Error %5 occurred while opening logfile %4.
.

MessageId=456
SymbolicName=PRIMARY_PAGE_READ_FAIL_ID
Language=English
%1 (%2) %3The primary header page of file %4 was damaged. The shadow header page (%5 bytes) was used instead.
.

MessageId=457
SymbolicName=EXISTING_LOG_FILE_HAS_BAD_SIGNATURE_ERROR_ID_2
Language=English
%1 (%2) %3The log signature of the existing logfile %4 doesn't match the logfiles from the backup set. Logfile replay cannot succeed unless all signatures match.
.

MessageId=458
SymbolicName=EXISTING_LOG_FILE_NOT_CONTIGUOUS_ERROR_ID_2
Language=English
%1 (%2) %3The logfiles %4 and %5 are not in a valid sequence. Logfile replay cannot succeed if there are gaps in the sequence of available logfiles.
.

MessageId=459
SymbolicName=BACKUP_LOG_FILE_MISSING_ERROR_ID
Language=English
%1 (%2) %3The file %4 is missing and could not be backed-up.
.

MessageId=460
SymbolicName=LOG_TORN_WRITE_DURING_HARD_RESTORE_ID
Language=English
%1 (%2) %3A torn-write was detected while restoring from backup in logfile %4 of the backup set. The failing checksum record is located at position %5. This logfile has been damaged and is unusable.
.

MessageId=461
SymbolicName=LOG_TORN_WRITE_DURING_HARD_RECOVERY_ID
Language=English
%1 (%2) %3A torn-write was detected during hard recovery in logfile %4. The failing checksum record is located at position %5. This logfile has been damaged and is unusable.
.

MessageId=462
SymbolicName=LOG_TORN_WRITE_DURING_SOFT_RECOVERY_ID
Language=English
%1 (%2) %3A torn-write was detected during soft recovery in logfile %4. The failing checksum record is located at position %5. The logfile damage will be repaired and recovery will continue to proceed.
.

MessageId=463
SymbolicName=LOG_CORRUPTION_DURING_HARD_RESTORE_ID
Language=English
%1 (%2) %3Corruption was detected while restoring from backup logfile %4. The failing checksum record is located at position %5. Data not matching the log-file fill pattern first appeared in sector %6. This logfile has been damaged and is unusable.
.

MessageId=464
SymbolicName=LOG_CORRUPTION_DURING_HARD_RECOVERY_ID
Language=English
%1 (%2) %3Corruption was detected during hard recovery in logfile %4. The failing checksum record is located at position %5. Data not matching the log-file fill pattern first appeared in sector %6. This logfile has been damaged and is unusable.
.

MessageId=465
SymbolicName=LOG_CORRUPTION_DURING_SOFT_RECOVERY_ID
Language=English
%1 (%2) %3Corruption was detected during soft recovery in logfile %4. The failing checksum record is located at position %5. Data not matching the log-file fill pattern first appeared in sector %6. This logfile has been damaged and is unusable.
.

MessageId=466
SymbolicName=LOG_USING_SHADOW_SECTOR_ID
Language=English
%1 (%2) %3An invalid checksum record in logfile %4 was patched using its shadow sector copy.
.

MessageId=467
SymbolicName=INDEX_CORRUPTED_ID
Language=English
%1 (%2) %3Database %6: Index %4 of table %5 is corrupted (%7).
.

MessageId=468
SymbolicName=LOG_FLUSH_WRITE_5_ERROR_ID
Language=English
%1 (%2) %3Unable to write to section 5 while flushing logfile %4. Error %5.
.

MessageId=470
SymbolicName=DB_PARTIALLY_ATTACHED_ID
Language=English
%1 (%2) %3Database %4 is partially attached. Attachment stage: %5. Error: %6.
.

MessageId=471
SymbolicName=UNDO_FAILED_ID
Language=English
%1 (%2) %3Unable to rollback operation #%4 on database %5. Error: %6. All future database updates will be rejected.
.

MessageId=472
SymbolicName=SHADOW_PAGE_READ_FAIL_ID
Language=English
%1 (%2) %3The shadow header page of file %4 was damaged. The primary header page (%5 bytes) was used instead.
.

MessageId=473
SymbolicName=DB_PARTIALLY_DETACHED_ID
Language=English
%1 (%2) %3Database %4 was partially detached.  Error %5 encountered updating database headers.
.

MessageId=474
SymbolicName=DATABASE_PAGE_CHECKSUM_MISMATCH_ID
Language=English
%1 (%2) %3The database page read from the file "%4" at offset %5 (database page %10) for %6 bytes failed verification due to a page checksum mismatch.  The stored checksum was %8 and the computed checksum was %9.  The read operation will fail with error %7.  If this condition persists then please restore the database from a previous backup.  This problem is likely due to faulty hardware. Please contact your hardware vendor for further assistance diagnosing the problem.
.

MessageId=475
SymbolicName=DATABASE_PAGE_NUMBER_MISMATCH_ID
Language=English
%1 (%2) %3The database page read from the file "%4" at offset %5 for %6 bytes failed verification due to a page number mismatch.  The expected page number was %8 and the stored page number was %9.  The read operation will fail with error %7.  If this condition persists then please restore the database from a previous backup. This problem is likely due to faulty hardware. Please contact your hardware vendor for further assistance diagnosing the problem.
.

MessageId=476
SymbolicName=DATABASE_PAGE_DATA_MISSING_ID
Language=English
%1 (%2) %3The database page read from the file "%4" at offset %5 (database page %8) for %6 bytes failed verification because it contains no page data.  The read operation will fail with error %7.  If this condition persists then please restore the database from a previous backup. This problem is likely due to faulty hardware. Please contact your hardware vendor for further assistance diagnosing the problem.
.

MessageId=477
SymbolicName=LOG_RANGE_CHECKSUM_MISMATCH_ID
Language=English
%1 (%2) %3The log range read from the file "%4" at offset %5 for %6 bytes failed verification due to a range checksum mismatch.  The expected checksum was %8 and the actual checksum was %9. The read operation will fail with error %7.  If this condition persists then please restore the logfile from a previous backup.
.

MessageId=478
SymbolicName=SLV_PAGE_CHECKSUM_MISMATCH_ID
Language=English
%1 (%2) %3The streaming page read from the file "%4" at offset %5 for %6 bytes failed verification due to a page checksum mismatch.  The expected checksum was %8 and the actual checksum was %9.  The read operation will fail with error %7.  If this condition persists then please restore the database from a previous backup.
.

MessageId=479
SymbolicName=PATCH_PAGE_CHECKSUM_MISMATCH_ID
Language=English
%1 (%2) %3The patch page read from the file "%4" at offset %5 for %6 bytes failed verification due to a page checksum mismatch.  The expected checksum was %8 and the actual checksum was %9.  The read operation will fail with error %7.  If this condition persists then please restore using an earlier backup set.
.

MessageId=480
SymbolicName=PATCH_PAGE_NUMBER_MISMATCH_ID
Language=English
%1 (%2) %3The patch page read from the file "%4" at offset %5 for %6 bytes failed verification due to a page number mismatch.  The expected page number was %8 and the actual page number was %9.  The read operation will fail with error %7.  If this condition persists then please restore using an earlier backup set.
.

MessageId=481
SymbolicName=OSFILE_READ_ERROR_ID
Language=English
%1 (%2) %3An attempt to read from the file "%4" at offset %5 for %6 bytes failed after %10 seconds with system error %8: "%9".  The read operation will fail with error %7.  If this error persists then the file may be damaged and may need to be restored from a previous backup.
.

MessageId=482
SymbolicName=OSFILE_WRITE_ERROR_ID
Language=English
%1 (%2) %3An attempt to write to the file "%4" at offset %5 for %6 bytes failed after %10 seconds with system error %8: "%9".  The write operation will fail with error %7.  If this error persists then the file may be damaged and may need to be restored from a previous backup.
.

MessageId=483
SymbolicName=OSFS_CREATE_FOLDER_ERROR_ID
Language=English
%1 (%2) %3An attempt to create the folder "%4" failed with system error %6: "%7".  The create folder operation will fail with error %5.
.

MessageId=484
SymbolicName=OSFS_REMOVE_FOLDER_ERROR_ID
Language=English
%1 (%2) %3An attempt to remove the folder "%4" failed with system error %6: "%7".  The remove folder operation will fail with error %5.
.

MessageId=485
SymbolicName=OSFS_DELETE_FILE_ERROR_ID
Language=English
%1 (%2) %3An attempt to delete the file "%4" failed with system error %6: "%7".  The delete file operation will fail with error %5.
.

MessageId=486
SymbolicName=OSFS_MOVE_FILE_ERROR_ID
Language=English
%1 (%2) %3An attempt to move the file "%4" to "%5" failed with system error %7: "%8".  The move file operation will fail with error %6.
.

MessageId=487
SymbolicName=OSFS_COPY_FILE_ERROR_ID
Language=English
%1 (%2) %3An attempt to copy the file "%4" to "%5" failed with system error %7: "%8".  The copy file operation will fail with error %6.
.

MessageId=488
SymbolicName=OSFS_CREATE_FILE_ERROR_ID
Language=English
%1 (%2) %3An attempt to create the file "%4" failed with system error %6: "%7".  The create file operation will fail with error %5.
.

MessageId=489
SymbolicName=OSFS_OPEN_FILE_RO_ERROR_ID
Language=English
%1 (%2) %3An attempt to open the file "%4" for read only access failed with system error %6: "%7".  The open file operation will fail with error %5.
.

MessageId=490
SymbolicName=OSFS_OPEN_FILE_RW_ERROR_ID
Language=English
%1 (%2) %3An attempt to open the file "%4" for read / write access failed with system error %6: "%7".  The open file operation will fail with error %5.
.

MessageId=491
SymbolicName=OSFS_SECTOR_SIZE_ERROR_ID
Language=English
%1 (%2) %3An attempt to determine the minimum I/O block size for the volume "%4" containing "%5" failed with system error %7: "%8".  The operation will fail with error %6.
.

MessageId=492
SymbolicName=LOG_DOWN_ID
Language=English
%1 (%2) %3The logfile sequence in "%4" has been halted due to a fatal error.  No further updates are possible for the databases that use this logfile sequence.  Please correct the problem and restart or restore from backup.
.

MessageId=493
SymbolicName=TRANSIENT_READ_ERROR_DETECTED_ID
Language=English
%1 (%2) %3A read operation on the file "%4" at offset %5 for %6 bytes failed %7 times over an interval of %8 seconds before finally succeeding.  More specific information on these failures was reported previously.  Transient failures such as these can be a precursor to a catastrophic failure in the storage subsystem containing this file.
.

MessageId=494
SymbolicName=ATTACHED_DB_MISMATCH_END_OF_RECOVERY_ID
Language=English
%1 (%2) %3Database recovery failed with error %4 because it encountered references to a database, '%5', which is no longer present. The database was not brought to a Clean Shutdown state before it was removed (or possibly moved or renamed). The database engine will not permit recovery to complete for this instance until the missing database is re-instated. If the database is truly no longer available and no longer required, procedures for recovering from this error are available in the Microsoft Knowledge Base or by following the "more information" link at the bottom of this message.
.

MessageId=495
SymbolicName=ATTACHED_DB_MISMATCH_DURING_RECOVERY_ID
Language=English
%1 (%2) %3Database recovery on '%5' failed with error %4. The database is not in the state expected at the first reference of this database in the log files. It is likely that a file copy of this database was restored, but not all log files since the file copy was made are currently available. Procedures for recovering from this error are available in the Microsoft Knowledge Base or by following the "more information" link at the bottom of this message.
.

MessageId=496
SymbolicName=REDO_HIGH_LOG_MISMATCH_ERROR_ID
Language=English
%1 (%2) %3Database %4 requires logfile %5 created at %6 in order to recover successfully. Recovery found the logfile created at %7.
.

MessageId=497
SymbolicName=DATABASE_HEADER_ERROR_ID
Language=English
%1 (%2) %3From information provided by the headers of %4, the file is not a database file. The headers of the file may be corrupted.
.

MessageId=498
SymbolicName=DELETE_LOG_FILE_TOO_NEW_ID
Language=English
%1 (%2) %3There is a gap in log sequence numbers, last used log file has generation %4. Logfiles %5 to %6 have been deleted so that recovery can complete.
.

MessageId=499
SymbolicName=DELETE_LAST_LOG_FILE_TOO_OLD_ID
Language=English
%1 (%2) %3There is a gap in log sequence numbers, last used log file has generation %4. Logfile %5 (generation %6) have been deleted so that recovery can complete.
.

MessageId=500
SymbolicName=REPAIR_BAD_PAGE_ID
Language=English
%1 (%2) %3The database engine lost one page of bad data. It is highly recommended that an application-level integrity check of the database be run to ensure application-level data integrity.
.

MessageId=501
SymbolicName=REPAIR_PAGE_LINK_ID
Language=English
%1 (%2) %3The database engine repaired one page link.
.

MessageId=502
SymbolicName=REPAIR_BAD_COLUMN_ID
Language=English
%1 (%2) %3The database engine lost one or more bad columns of data in one record. It is highly recommended that an application-level integrity check of the database be run to ensure application-level data integrity.
.

MessageId=503
SymbolicName=REPAIR_BAD_RECORD_ID
Language=English
%1 (%2) %3The database engine lost one bad data record. It is highly recommended that an application-level integrity check of the database be run to ensure application-level data integrity.
.

MessageId=504
SymbolicName=REPAIR_BAD_TABLE
Language=English
%1 (%2) %3The database engine lost one table called %4. It is highly recommended that an application-level integrity check of the database be run to ensure application-level data integrity.
.

MessageId=505
SymbolicName=OSFS_OPEN_COMPRESSED_FILE_RW_ERROR_ID
Language=English
%1 (%2) %3An attempt to open the compressed file "%4" for read / write access failed because it could not be converted to a normal file.  The open file operation will fail with error %5.  To prevent this error in the future you can manually decompress the file and change the compression state of the containing folder to uncompressed.  Writing to this file when it is compressed is not supported.
.

MessageId=506
SymbolicName=OSFS_DISK_SPACE_ERROR_ID
Language=English
%1 (%2) %3An attempt to determine the amount of free/total space for the volume "%4" containing "%5" failed with system error %7: "%8".  The operation will fail with error %6.
.

MessageId=507
SymbolicName=OSFILE_READ_TOO_LONG_ID
Language=English
%1 (%2) %3A request to read from the file "%4" at offset %5 for %6 bytes succeeded, but took an abnormally long time (%7 seconds) to be serviced by the OS. This problem is likely due to faulty hardware. Please contact your hardware vendor for further assistance diagnosing the problem.
.

MessageId=508
SymbolicName=OSFILE_WRITE_TOO_LONG_ID
Language=English
%1 (%2) %3A request to write to the file "%4" at offset %5 for %6 bytes succeeded, but took an abnormally long time (%7 seconds) to be serviced by the OS. This problem is likely due to faulty hardware. Please contact your hardware vendor for further assistance diagnosing the problem.
.

MessageId=509
SymbolicName=OSFILE_READ_TOO_LONG_AGAIN_ID
Language=English
%1 (%2) %3A request to read from the file "%4" at offset %5 for %6 bytes succeeded, but took an abnormally long time (%7 seconds) to be serviced by the OS. In addition, %8 other I/O requests to this file have also taken an abnormally long time to be serviced since the last message regarding this problem was posted %9 seconds ago. This problem is likely due to faulty hardware. Please contact your hardware vendor for further assistance diagnosing the problem.
.

MessageId=510
SymbolicName=OSFILE_WRITE_TOO_LONG_AGAIN_ID
Language=English
%1 (%2) %3A request to write to the file "%4" at offset %5 for %6 bytes succeeded, but took an abnormally long time (%7 seconds) to be serviced by the OS. In addition, %8 other I/O requests to this file have also taken an abnormally long time to be serviced since the last message regarding this problem was posted %9 seconds ago. This problem is likely due to faulty hardware. Please contact your hardware vendor for further assistance diagnosing the problem.
.

MessageId=512
SymbolicName=TASK_INFO_STATS_TOO_LONG_ID
Language=English
%1 (%2) %3The system has serviced a %4 task but it took an abnormally long time (%5 seconds to be dispatched, %6 seconds to be executed).

.

MessageId=513
SymbolicName=TASK_INFO_STATS_TOO_LONG_AGAIN_ID
Language=English
%1 (%2) %3The system has serviced a %4 task but it took an abnormally long time (%5 seconds to be dispatched, %6 seconds to be executed). In addition, %7 other %4 tasks have also taken an abnormally long time to be serviced since the last message regarding this problem was posted %8 seconds ago. 

.

MessageId=514
SymbolicName=ALMOST_OUT_OF_LOG_SEQUENCE_ID
Language=English
%1 (%2) %3Log sequence numbers for this instance have almost been completely consumed. The current log generation is %4 which is approaching the maximum log generation of %5, there are %6 log generations left to be used. To begin renumbering from generation 1, the instance must be shutdown cleanly and all log files must be deleted. Backups will be invalidated.
.

MessageId=515
SymbolicName=FLUSH_DEPENDENCY_CORRUPTED_ID
Language=English
%1 (%2) %3Database %4: Page %5 failed verification due to a flush-order dependency mismatch.  This page should have flushed before page %6, but the latter page has instead flushed first. Recovery/restore will fail with error %7. If this condition persists then please restore the database from a previous backup. This problem is likely due to faulty hardware "losing" one or more flushes on one or both of these pages sometime in the past. Please contact your hardware vendor for further assistance diagnosing the problem.
.

MessageId=516
SymbolicName=DBTIME_MISMATCH_ID
Language=English
%1 (%2) %3Database %4: Page %5 failed verification due to a timestamp mismatch at log position %10 (currently replaying log position %12).  The before/after timestamps in the log record were %6/%9 but the actual timestamp on the page was %7.  Recovery/restore will fail with error %8.  If this condition persists then please restore the database from a previous backup. This problem is likely due to faulty hardware "losing" one or more flushes on this page or the database header at some time in the past. Please contact your hardware vendor for further assistance diagnosing the problem.
%nAdditional information:
%n%tWithin initial required range: %11
%n%tTotal number of pages affected: %13
.

MessageId=517
SymbolicName=CONSISTENT_TIME_MISMATCH_ID
Language=English
%1 (%2) %3Database recovery failed with error %4 because it encountered references to a database, '%5', which does not match the current set of logs. The database engine will not permit recovery to complete for this instance until the mismatching database is re-instated. If the database is truly no longer available or no longer required, procedures for recovering from this error are available in the Microsoft Knowledge Base or by following the "more information" link at the bottom of this message.
.

MessageId=518
SymbolicName=LOG_FILE_MISSING_ID
Language=English
%1 (%2) %3The log file %4 is missing (error %5) and cannot be used. If this log file is required for recovery, a good copy of the log file will be needed for recovery to complete successfully.
.

MessageId=519
SymbolicName=LOG_RANGE_MISSING_ID
Language=English
%1 (%2) %3The range of log files %4 to %5 is missing (error %6) and cannot be used. If these log files are required for recovery, a good copy of these log files will be needed for recovery to complete successfully.
.

MessageId=520
SymbolicName=BACKUP_LOG_FILE_NOT_COPIED_ID
Language=English
%1 (%2) %3Log truncation failed because not all log files required for recovery were successfully copied.
.

MessageId=521
SymbolicName=LOG_FILE_SIGNAL_ERROR_ID
Language=English
%1 (%2) %3Unable to schedule a log write on log file %4. Error %5.
.

MessageId=522
SymbolicName=OSFS_OPEN_DEVICE_ERROR_ID
Language=English
%1 (%2) %3An attempt to open the device with name "%4" containing "%5" failed with system error %7: "%8". The operation will fail with error %6.
.

MessageId=523
SymbolicName=REDO_MISSING_COMMITTED_LOGS_BUT_HAS_LOSSY_RECOVERY_OPTION_ID
Language=English
%1 (%2) %3Database %4 requires log files %5-%6 in order to recover all committed data.  Recovery could only locate up to log file: %7, and could not locate log generation %8.  If the log file cannot be found, the database(s) can be recovered manually, but will result in losing committed data.
%nDetails:
%n%tLog directory: %9
.

MessageId=524
SymbolicName=REDO_MISSING_COMMITTED_LOGS_LOST_BUT_CONSISTENT_ID
Language=English
%1 (%2) %3Database %4 has lost %5 committed log files (%6-%7). However, lost log resilience has allowed ESE to recover this database to a consistent state, but with data loss.  Recovery could only locate log files up to %8.
%nDetails:
%n%tLog directory: %9
.

MessageId=525
SymbolicName=LOG_FILE_CORRUPTED_BUT_HAS_LOSSY_RECOVERY_OPTION_ID
Language=English
%1 (%2) %3The log file %4 is damaged, invalid, or inaccessible (error %5) and cannot be used. If the log file cannot be found, the database(s) can still be recovered manually, but will result in losing committed data.
%nDetails:
%n%tLog directory: %6
.

MessageId=526
SymbolicName=REDO_CORRUPT_COMMITTED_LOG_REMOVED_ID
Language=English
%1 (%2) %3The log file %5 (generation %6) has damaged or invalid log (%7), ESE has removed the portion of log corrupted since these committed logs are specified as unneeded, so that ESE can recover (through generation %4) any attached databases to a consistent state, but with data loss.
.

MessageId=527
SymbolicName=REDO_MORE_COMMITTED_LOGS_REMOVED_ID
Language=English
%1 (%2) %3ESE has removed %4 committed log files (%5-%6) to maintain an in order log stream.  However lost log resilience has allowed ESE to recover to a consistent state, but with data loss.  Recovery could only recover through log files up to %7.
%nDetails:
%n%tLog directory: %8
.

MessageId=528
SymbolicName=REDO_COMMITTED_LOGS_OUTOFORDERMISSING_BUT_HAS_LOSSY_RECOVERY_OPTION_ID
Language=English
%1 (%2) %3Recovery could only locate up to log file %4 (generation %5) before detecting an out of sequence logs, and could not locate log file %6 (generation %7).  If the log file cannot be found, the database(s) can be recovered manually, but will result in losing committed data.
%nDetails:
%n%tLog directory: %8
.

MessageId=529
SymbolicName=LOG_CHECKSUM_REC_CORRUPTION_ID
Language=English
%1 (%2) %3The log range read from the file "%4" at offset %5 for %6 bytes failed verification due to a corrupted checksum log record. The read operation will fail with error %7.  If this condition persists, restore the logfile from a previous backup.
.

MessageId=530
SymbolicName=DATABASE_PAGE_LOST_FLUSH_ID
Language=English
%1 (%2) %3The database page read from the file "%4" at offset %5 (database page %8) for %6 bytes failed verification due to a lost flush detection timestamp mismatch. The read operation will fail with error %7.
%nThe flush state on database page %8 was %9 while the flush state on flush map page %10 was %11.
%nContext: %12.
%nIf this condition persists, restore the database from a previous backup. This problem is likely due to faulty hardware. Please contact your hardware vendor for further assistance diagnosing the problem.
.

MessageId=531
SymbolicName=SUSPECTED_BAD_BLOCK_OVERWRITE_ID
Language=English
%1 (%2) %3The database engine attempted a clean write operation on page %4 of database %5. This action was performed in an attempt to correct a previous problem reading from the page.
.

MessageId=532
SymbolicName=IOMGR_HUNG_READ_IO_DETECTED_ID
Language=English
%1 (%2) %3A request to read from the file "%4" at offset %5 for %6 bytes has not completed for %7 second(s). This problem is likely due to faulty hardware. Please contact your hardware vendor for further assistance diagnosing the problem.
.

MessageId=533
SymbolicName=IOMGR_HUNG_WRITE_IO_DETECTED_ID
Language=English
%1 (%2) %3A request to write to the file "%4" at offset %5 for %6 bytes has not completed for %7 second(s). This problem is likely due to faulty hardware. Please contact your hardware vendor for further assistance diagnosing the problem.
.

MessageId=534
SymbolicName=PATCH_REQUEST_ISSUE_FAILED
Language=English
%1 (%2) %3A patch request for file "%4" at page number "%5" has failed to be issued with error %6. If this condition persists then please restore using an earlier backup set.
.

MessageId=535
SymbolicName=OSFS_SECTOR_SIZE_LOOKUP_FAILED_ID
Language=English
%1 (%2) %3An attempt to determine the minimum I/O block size for the volume containing "%4" failed.  The default sector-size of 512 bytes will be used.
.

MessageId=536
SymbolicName=CREATING_TEMP_DB_FAILED_ID
Language=English
%1 (%2) %3An attempt to create temporary database %4 failed with error %5.
.

MessageId=537
SymbolicName=BAD_PAGE_EMPTY_SEEK_ID
Language=English
%1 (%2) %3A request for a node on an empty page (Pgno: %7, Flags: %9) has been made (error %4) for a B-Tree (ObjectId: %5, PgnoRoot: %6) of database %8. This is typically due to a lost I/O from 
storage hardware. Please check with your hardware vendor for latest firmware revisions, make changes to your controller's caching parameters, use crash consistent hardware with Forced
Unit Access support, and/or replace faulty hardware.
.

MessageId=538
SymbolicName=DBTIME_CHECK_MISMATCH_ACTIVE_DB_BEHIND_ID
Language=English
%1 (%2) %3Database %4: Page %5 in a B-Tree (ObjectId: %12) failed verification due to a timestamp mismatch at log position %10.  The remote ("before") timestamp persisted to the log record was %6 but the actual timestamp on the page was %7 (note: DbtimeCurrent = %9). This indicates the active node lost a flush (error %8). This problem is likely due to faulty hardware "losing" one or more flushes on this page sometime in the past on the active node. Please contact your hardware vendor for further assistance diagnosing the problem.%n
Additional information:%n
%tSource: %11%n
.

MessageId=539
SymbolicName=DBTIME_CHECK_MISMATCH_PASSIVE_DB_BEHIND_ID
Language=English
%1 (%2) %3Database %4: Page %5 in a B-Tree (ObjectId: %12) failed verification due to a timestamp mismatch at log position %10.  The remote ("before") timestamp persisted to the log record was %6 but the actual timestamp on the page was %7 (note: DbtimeCurrent = %9). This indicates the passive node lost a flush (error %8). This problem is likely due to faulty hardware "losing" one or more flushes on this page sometime in the past on the passive node. Please contact your hardware vendor for further assistance diagnosing the problem.%n
Additional information:%n
%tSource: %11%n
.

MessageId=540
SymbolicName=DB_DIVERGENCE_ID
Language=English
%1 (%2) %3Database %4: Page %5 in a B-Tree (ObjectId: %11) logical data checksum %6 failed to match logged scan check %7 checksum (seed %8) at log position %9.%n
Additional information:%n
%tSource: %10%n
.

MessageId=541
SymbolicName=DB_DIVERGENCE_UNINIT_PAGE_PASSIVE_DB_ID
Language=English
%1 (%2) %3Database %4: Page %5 appears to be uninitialized at log position %8 on the current node.  The remote ("before") timestamp persisted to the log record was %6 (note: DbtimeCurrent = %7). This indicates the passive node lost a flush. This problem is likely due to faulty hardware "losing" one or more flushes on this page sometime in the past on the passive node. Please contact your hardware vendor for further assistance diagnosing the problem.%n
Additional information:%n
%tSource: %9%n
.

MessageId=542
SymbolicName=DATABASE_PAGE_LOGICALLY_CORRUPT_ID
Language=English
%1 (%2) %3Database %4: Page %5 (ObjectId: %6) has a logical corruption of type '%7'.
%nDetails: %8
.

MessageId=543
SymbolicName=LOG_LAST_SEGMENT_CHECKSUM_MISMATCH_ID
Language=English
%1 (%2) %3Previous log's accumulated segment checksum mismatch in logfile %4, Expected: %5, Actual: %6.
.

MessageId=544
SymbolicName=DATABASE_PAGE_PERSISTED_LOST_FLUSH_ID
Language=English
%1 (%2) %3The database page read from the file "%4" at offset %5 (database page %8) for %6 bytes failed verification due to a persisted lost flush detection timestamp mismatch. The read operation will fail with error %7.
%nThe flush state on database page %8 was %9 while the flush state on flush map page %10 was %11.
%nContext: %12.
%nIf this condition persists, restore the database from a previous backup. This problem is likely due to faulty hardware. Please contact your hardware vendor for further assistance diagnosing the problem.
.

MessageId=545
SymbolicName=DB_DIVERGENCE_UNINIT_PAGE_ACTIVE_DB_ID
Language=English
%1 (%2) %3Database %4: Page %5 in a B-Tree (ObjectId: %12) appears to be uninitialized at log position %10 on the remote node.  The remote ("before") timestamp persisted to the log record was %6 but the actual timestamp on the page was %7 (note: DbtimeCurrent = %9). This indicates the active node lost a flush (error %8). This problem is likely due to faulty hardware "losing" one or more flushes on this page sometime in the past on the active node. Please contact your hardware vendor for further assistance diagnosing the problem.%n
Additional information:%n
%tSource: %11%n
.

MessageId=546
SymbolicName=LOG_VERSION_TOO_LOW_FOR_ENGINE_ID
Language=English
%1 (%2) %3The log generation (%4) is too low (%5) to be supported or replayed by this database engine (min supported %6).
.

MessageId=547
SymbolicName=LOG_VERSION_TOO_HIGH_FOR_ENGINE_ID
Language=English
%1 (%2) %3The log generation (%4) is too high (%5) to be supported or replayed by this database engine (max supported %6).
.

MessageId=548
SymbolicName=LOG_VERSION_TOO_HIGH_FOR_PARAM_ID
Language=English
%1 (%2) %3The log generation's [%4] version %5 is higher than the maximum version configured by the application %6. Current engine format version parameter setting: %7
.

MessageId=549
SymbolicName=BAD_LINE_COUNT_ID
Language=English
%1 (%2) %3Database %4 has a page (pgno %5) with too few lines in a B-Tree (ObjectId: %6, PgnoRoot: %7), operation attempted on line %8, actual number of lines on page %9.
.

MessageId=550
SymbolicName=COMPRESSED_LVCHUNK_CHECKSUM_MISMATCH_ID
Language=English
%1 (%2) %3Database %4: A compressed LV chunk failed verification during decompression due to a checksum mismatch. This indicates a logical corruption in the compress/decompress pipeline.
pgnoFDP = %5. Key = %6.
.

MessageId=551
SymbolicName=OSFILE_FLUSH_ERROR_ID
Language=English
%1 (%2) %3An attempt to flush to the file/disk buffers of "%4" failed after %5 seconds with system error %6: "%7".  The flush operation will fail with error %8.  If this error persists then the file system may be damaged and may need to be restored from a previous backup.
.

MessageId=552
SymbolicName=LOG_CORRUPT_ID
Language=English
%1 (%2) %3The log file at "%4" is corrupt with reason '%5'. Last valid segment was %6, current segment is %7. The expected checksum was %8 and the actual checksum was %9. The read completed with error-code %10.  If this condition persists then please restore the logfile from a previous backup.
.

MessageId=553
SymbolicName=RSTMAP_FAIL_ID
Language=English
%1 (%2) %3Failed looking up restore-map entry for database %4 with unexpected error %5.
.

MessageId=554
SymbolicName=LOG_SEGMENT_CHECKSUM_CORRECTED_ID
Language=English
%1 (%2) %3The log segment read from the file "%4" at offset %6 (segment number %5) failed verification. Bit %7 was corrupted and has been corrected.  This problem is likely due to faulty hardware and may continue. Transient failures such as these can be a precursor to a catastrophic failure in the storage subsystem containing this file.  Please contact your hardware vendor for further assistance diagnosing the problem.
.

MessageId=555
SymbolicName=RECOVERY_DATABASE_PAGE_DATA_MISSING_ID
Language=English
%1 (%2) %3Database %4: Page %5 failed verification due not containing any data at log position %6 (currently replaying log position %7).  Recovery/restore will fail with error %8.  If this condition persists then please restore the database from a previous backup. This problem is likely due to faulty hardware "losing" one or more flushes on this page some time in the past. Please contact your hardware vendor for further assistance diagnosing the problem.
%nAdditional information:
%n%tWithin initial required range: %9
%n%tTotal number of pages affected: %10
.

MessageId=556
SymbolicName=DECOMPRESSION_FAILED
Language=English
%1 (%2) %3Database %4: Data in line %5 on pgno %6 in a B-Tree (ObjectId: %7, PgnoRoot: %8) failed to decompress.
.

MessageId=557
SymbolicName=DB_DIVERGENCE_BEYOND_EOF_PAGE_PASSIVE_DB_ID
Language=English
%1 (%2) %3Database %4: Page %5 was shown to be beyond the end of file at log position %8, while the remote ("before") timestamp persisted to the log record was %6 (note: DbtimeCurrent = %7).%n
Additional information:%n
%tSource: %9%n
.

MessageId=558
SymbolicName=GAP_LOG_FILE_GEN_ID
Language=English
%1 (%2) %3There is a gap in sequence number following logfile generation %4. Highest logfile generation present is %5.
.

MessageId=559
SymbolicName=DBTIME_CHECK_ACTIVE_CORRUPTED_ID
Language=English
%1 (%2) %3Database %4: A timestamp inconsistency has been detected while scanning page %5 (ObjectId: %10) on the active node at log position %6.  The timestamp on the page is too advanced.
Additional information:%n
%tTimestamp persisted to the page: %7%n
%tGlobal timestamp of the database: %8%n
%tSource: %9%n
.

MessageId=560
SymbolicName=DBTIME_CHECK_PASSIVE_CORRUPTED_ID
Language=English
%1 (%2) %3Database %4: A timestamp inconsistency has been detected while verifying page %5 in a B-Tree (ObjectId: %12) at log position %10.  The timestamp on the page is too advanced.
Additional information:%n
%tTimestamp persisted to the page: %7%n
%tGlobal timestamp of the database: %9%n
%tTimestamp persisted to the log record: %6%n
%tError code: %8%n
%tSource: %11%n
.

MessageId=561
SymbolicName=CORSICA_INIT_FAILED_ID
Language=English
%1 (%2) %3Corsica device failed to initialize with status code '%4' with reason '%5'.
.

MessageId=562
SymbolicName=CORSICA_REQUEST_FAILED_ID
Language=English
%1 (%2) %3A request to corsica device for '%7' failed with status code '%4', engine code '%5' with reason '%6'.
.

MessageId=563
SymbolicName=DATABASE_LEAKED_ROOT_SPACE_ID
Language=English
%1 (%2) %3Database '%4' encountered error %5 while releasing space to the database root (ObjectId %6). A total of %7 page(s) (%8 - %9) will be leaked. The leaked space may be reclaimed by running offline defragmentation on the database.%n
Tag: %10%n
.

MessageId=564
SymbolicName=DATABASE_LEAKED_NON_ROOT_SPACE_ID
Language=English
%1 (%2) %3Database '%4' encountered error %5 while releasing space to ObjectId %6. A total of %7 page(s) (%8 - %9) will be leaked. The leaked space may be reclaimed by either running offline defragmentation on the database or deleting and re-creating the object.%n
Tag: %10%n
.

MessageId=565
SymbolicName=RECOVERY_DATABASE_BAD_REVERTED_PAGE_ID
Language=English
%1 (%2) %3Database %4: Page %5 failed verification due to being reverted to a new page using revert snapshot, but the log record at log position %6 expected the page to not be a new page (currently replaying log position %7).  Recovery/restore will fail with error %8.  This problem is likely due to a bad revert operation.
%nAdditional information:
%n%tWithin initial required range: %9
%n%tTotal number of pages affected: %10
.


;// !!! ARE YOU SURE you're adding this in the right place !!! ???


;///////////////////////////////////////////////////////////
;//	Not sure what this section is for?
;///////////////////////////////////////////////////////////
;// Seems to be systematic errors that are not corruptions

MessageId=600
SymbolicName=CONVERT_DUPLICATE_KEY
Language=English
%1 (%2) %3The database engine encountered an unexpected duplicate key on the table %4. One record has been dropped.
.

MessageId=601
SymbolicName=FUNCTION_NOT_FOUND_ERROR_ID
Language=English
%1 (%2) %3The database engine could not find the entrypoint %4 in the file %5.
.

MessageId=602
SymbolicName=MANY_LOST_COMPACTION_ID
Language=English
%1 (%2) %3Database '%4': Background clean-up skipped pages. The database may benefit from widening the online maintenance window during off-peak hours. If this message persists, offline defragmentation may be run to remove all skipped pages from the database. Discarded RCE id:%5, trxBegin0:%6, trxCommit0: %7, last trxNewest: %8, current trxNewest: %9.
.

MessageId=603
SymbolicName=SPACE_LOST_ON_FREE_ID
Language=English
%1 (%2) %3Database '%4': The database engine lost unused pages %5-%6 while attempting space reclamation on a B-Tree (ObjectId %7). The space may not be reclaimed again until offline defragmentation is performed.
.

MessageId=604
SymbolicName=LANGUAGE_NOT_SUPPORTED_ID
Language=English
%1 (%2) %3Locale ID %4 (%5 %6) is either invalid, not installed on this machine or could not be validated with system error %7: "%8". The operation will fail with error %9.
.

MessageId=605
SymbolicName=CONVERT_COLUMN_TO_TAGGED_ID
Language=English
%1 (%2) %3Column '%4' of Table '%5' was converted to a Tagged column.
.

MessageId=606
SymbolicName=CONVERT_COLUMN_TO_NONTAGGED_ID
Language=English
%1 (%2) %3Column '%4' of Table '%5' was converted to a non-Tagged column.
.

MessageId=607
SymbolicName=CONVERT_COLUMN_BINARY_TO_LONGBINARY_ID
Language=English
%1 (%2) %3Column '%4' of Table '%5' was converted from Binary to LongBinary.
.

MessageId=608
SymbolicName=CONVERT_COLUMN_TEXT_TO_LONGTEXT_ID
Language=English
%1 (%2) %3Column '%4' of Table '%5' was converted from Text to LongText.
.

MessageId=609
SymbolicName=START_INDEX_CLEANUP_KNOWN_VERSION_ID
Language=English
%1 (%2) %3The database engine is initiating index cleanup of database '%4' as a result of a Windows version upgrade from %5.%6.%7 SP%8 to %9.%10.%11 SP%12. This message is informational and does not indicate a problem in the database.
.

MessageId=610
SymbolicName=START_INDEX_CLEANUP_UNKNOWN_VERSION_ID
Language=English
%1 (%2) %3The database engine is initiating index cleanup of database '%4' as a result of a Windows version upgrade to %5.%6.%7 SP%8. This message is informational and does not indicate a problem in the database.
.

MessageId=611
SymbolicName=DO_SECONDARY_INDEX_CLEANUP_ID
Language=English
%1 (%2) %3Database '%4': The secondary index '%5' of table '%6' will be rebuilt as a precautionary measure after the Windows version upgrade of this system. This message is informational and does not indicate a problem in the database.
.

MessageId=612
SymbolicName=STOP_INDEX_CLEANUP_ID
Language=English
%1 (%2) %3The database engine has successfully completed index cleanup on database '%4'.
.

MessageId=613
SymbolicName=PRIMARY_INDEX_OUT_OF_DATE_ERROR_ID
Language=English
%1 (%2) %3Database '%4': The primary index '%5' of table '%6' is out of date with sorting libraries. If used in this state (i.e. not rebuilt), it may appear corrupt or get further corrupted. If there is no later event showing the index being rebuilt, then please defragment the database to rebuild the index.
.

MessageId=614
SymbolicName=SECONDARY_INDEX_OUT_OF_DATE_ERROR_ID
Language=English
%1 (%2) %3Database '%4': The secondary index '%5' of table '%6' is out of date with sorting libraries. If used in this state (i.e. not rebuilt), it may appear corrupt or get further corrupted. If there is no later event showing the index being rebuilt, then please defragment the database to rebuild the index.
.

MessageId=615
SymbolicName=START_FORMAT_UPGRADE_ID
Language=English
%1 (%2) %3The database engine is converting the database '%4' from format %5 to format %6.
.

MessageId=616
SymbolicName=STOP_FORMAT_UPGRADE_ID
Language=English
%1 (%2) %3The database engine has successfully converted the database '%4' from format %5 to format %6.
.

MessageId=617
SymbolicName=CONVERT_INCOMPLETE_ERROR_ID
Language=English
%1 (%2) %3Attempted to use database '%4', but conversion did not complete successfully on this database. Please restore from backup and re-run database conversion.
.

MessageId=618
SymbolicName=BUILD_SPACE_CACHE_ID
Language=English
%1 (%2) %3Database '%4': The database engine has built an in-memory cache of %5 space tree nodes on a B-Tree (ObjectId: %6, PgnoRoot: %7) to optimize space requests for that B-Tree. The space cache was built in %8 milliseconds. This message is informational and does not indicate a problem in the database.
.

MessageId=619
SymbolicName=ATTACH_TO_BACKUP_SET_DATABASE_ERROR_ID
Language=English
%1 (%2) %3Attempted to attach database '%4' but it is a database restored from a backup set on which hard recovery was not started or did not complete successfully.
.

MessageId=620
SymbolicName=RECORD_FORMAT_CONVERSION_FAILED_ID
Language=English
%1 (%2) %3Unable to convert record format for record %4:%5:%6.
.

MessageId=621
SymbolicName=OUT_OF_OBJID
Language=English
%1 (%2) %3Database '%4': Out of B-Tree IDs (ObjectIDs). Freed/unused B-Tree IDs may be reclaimed by performing an offline defragmentation of the database.
.

MessageId=622
SymbolicName=ALMOST_OUT_OF_OBJID
Language=English
%1 (%2) %3Database '%4': Available B-Tree IDs (ObjectIDs) have almost been completely consumed. Freed/unused B-Tree IDs may be reclaimed by performing an offline defragmentation of the database.
.

MessageId=623
SymbolicName=VERSION_STORE_REACHED_MAXIMUM_ID
Language=English
%1 (%2) %3The version store for this instance (%4) has reached its maximum size of %5Mb. It is likely that a long-running transaction is preventing cleanup of the version store and causing it to build up in size. Updates will be rejected until the long-running transaction has been completely committed or rolled back.
%nPossible long-running transaction:
%n%tSessionId: %6
%n%tSession-context: %7
%n%tSession-context ThreadId: %8
%n%tCleanup: %9
%n%tSession-trace:
%n%10
.

MessageId=624
SymbolicName=VERSION_STORE_OUT_OF_MEMORY_ID
Language=English
%1 (%2) %3The version store for this instance (%4) cannot grow because it is receiving Out-Of-Memory errors from the OS. It is likely that a long-running transaction is preventing cleanup of the version store and causing it to build up in size. Updates will be rejected until the long-running transaction has been completely committed or rolled back.
%nCurrent version store size for this instance: %5Mb
%nMaximum version store size for this instance: %6Mb
%nGlobal memory pre-reserved for all version stores: %7Mb
%nPossible long-running transaction:
%n%tSessionId: %8
%n%tSession-context: %9
%n%tSession-context ThreadId: %10
%n%tCleanup: %11
%n%tSession-trace:
%n%12
.

MessageId=625
SymbolicName=INVALID_LCMAPFLAGS_ID
Language=English
%1 (%2) %3The database engine does not support the LCMapString() flags %4.
.

MessageId=626
SymbolicName=DO_MSU_CLEANUP_ID
Language=English
%1 (%2) %3The database engine updated %4 index entries in database %5 because of a change in the NLS version. This message is informational and does not indicate a problem in the database.
.

MessageId=627
SymbolicName=MSU_CLEANUP_FAIL_RO_ID
Language=English
%1 (%2) %3The database engine was unable to updated index entries in database %5 because the database is read-only.
.

MessageId=628
SymbolicName=BT_MOVE_SKIPPED_MANY_NODES_ID
Language=English
%1 (%2) %3Database '%4': While attempting to move to the next or
previous node in a B-Tree, the database engine took %17 milliseconds (waiting %18 milliseconds on faults) to skip over %7
non-visible nodes in %8 pages (faulting in %19 pages). It is likely that these non-visible
nodes are nodes which have been marked for deletion but which are
yet to be purged. The database may benefit from widening the online
maintenance window during off-peak hours in order to purge such nodes
and reclaim their space. If this message persists, offline
defragmentation may be run to remove all nodes which have been marked
for deletion but are yet to be purged from the database.
%n%tName: %5
%n%tOwning Table: %6
%n%tObjectId: %9
%n%tPgnoRoot: %10
%n%tTCE: %11
%n%tUnversioned Deletes: %12
%n%tUncommitted Deletes: %13
%n%tCommitted Deletes: %14
%n%tNon-Visible Inserts: %15
%n%tFiltered: %16
%n%tDbtime Distrib: %20
.

MessageId=629
SymbolicName=BT_MOVE_SKIPPED_MANY_NODES_AGAIN_ID
Language=English
%1 (%2) %3Database '%4': While attempting to move to the next or
previous node in a B-Tree, the database engine took %21 milliseconds (waiting %22 milliseconds on faults) to skip over %7
non-visible nodes in %8 pages (faulting in %23 pages). In addition, since this message was
last reported %16 hours ago, %17 other incidents of excessive
non-visible nodes were encountered (a total of %18 nodes in %19 pages
were skipped) during navigation in this B-Tree. It is likely that
these non-visible nodes are nodes which have been marked for deletion
but which are yet to be purged. The database may benefit from
widening the online maintenance window during off-peak hours in order
to purge such nodes and reclaim their space. If this message
persists, offline defragmentation may be run to remove all nodes
which have been marked for deletion but have yet to be purged from
the database.
%n%tName: %5
%n%tOwning Table: %6
%n%tObjectId: %9
%n%tPgnoRoot: %10
%n%tTCE: %11
%n%tUnversioned Deletes: %12
%n%tUncommitted Deletes: %13
%n%tCommitted Deletes: %14
%n%tNon-Visible Inserts: %15
%n%tFiltered: %20
%n%tDbtime Distrib: %24
.

MessageId=630
SymbolicName=VERSION_STORE_REACHED_MAXIMUM_TRX_SIZE
Language=English
%1 (%2) %3The version store for this instance (%4) has a long-running transaction that exceeds the maximum transaction size.
%nCurrent version store size for this instance: %5Mb
%nMaximum transaction size for this instance: %6Mb
%nMaximum version store size for this instance: %7Mb
%nLong-running transaction:
%n%tSessionId: %8
%n%tSession-context: %9
%n%tSession-context ThreadId: %10
%n%tCleanup: %11
%n%tSession-trace:
%n%12
.

MessageId=631
SymbolicName=DB_SECTOR_SIZE_MISMATCH
Language=English
%1 (%2) %3The file system reports that the database file at %4 has a sector size of %5 which is greater than the page size of %6.  This may result in higher torn write corruption incidence and/or in database corruption via lost flushes.
.

MessageId=632
SymbolicName=LOG_SECTOR_SIZE_MISMATCH
Language=English
%1 (%2) %3The file system reports that the log file at %4 has a sector size of %5 which is unsupported, using a sector size of 4096 instead.  This may result in transaction durability being lost.
.

MessageId=633
SymbolicName=RECOVERY_WAIT_READ_ONLY
Language=English
%1 (%2) %3Recovery had to pause (%4 ms) to allow an older read-only transaction to complete. If this condition persists, it can result in stale copies.
.

MessageId=634
SymbolicName=RECOVERY_LONG_WAIT_READ_ONLY
Language=English
%1 (%2) %3Recovery had to pause for a long time (%4 ms) to allow an older read-only transaction to complete. If this condition persists, it can result in stale copies.
.

MessageId=635
SymbolicName=FLUSHMAP_FILE_ATTACH_FAILED_ID
Language=English
%1 (%2) %3Failed to attach flush map file "%4" with error %5.
.

MessageId=636
SymbolicName=FLUSHMAP_FILE_DELETED_ID
Language=English
%1 (%2) %3Flush map file "%4" will be deleted. Reason: %5.
.

MessageId=637
SymbolicName=FLUSHMAP_FILE_CREATED_ID
Language=English
%1 (%2) %3New flush map file "%4" will be created to enable persisted lost flush detection.
.

MessageId=638
SymbolicName=FLUSHMAP_DATAPAGE_ERROR_VALIDATE_ID
Language=English
%1 (%2) %3Error %4 validating page %5 on flush map file "%6". The flush type of the database pages will be set to 'unknown' state.
.

MessageId=639
SymbolicName=FLUSHMAP_DBTIME_MISMATCH_ID
Language=English
%1 (%2) %3Inconsistent timestamp detected on page %4 of flush map file "%5" (empty if flush map persistence in disabled). The maximum timestamp on the flush map is %6, but database page %7 has a timestamp of %8. If flush map persistence is enabled, this problem is likely due to faulty hardware. Please contact your hardware vendor for further assistance diagnosing the problem.
.

MessageId=640
SymbolicName=FLUSHMAP_HEADER_ERROR_VALIDATE_ID
Language=English
%1 (%2) %3Error %4 validating header page on flush map file "%5". The flush map file will be invalidated.
%nAdditional information: %6
.

MessageId=641
SymbolicName=LOG_FORMAT_FEATURE_REFUSED_DUE_TO_ENGINE_FORMAT_VERSION_SETTING_DEPRECATED
Language=English
%1 (%2) %3The log format feature version %5 could not be used due to the current log format %6, controlled by the parameter %4.
.


MessageId=642
SymbolicName=DATABASE_FORMAT_FEATURE_REFUSED_DUE_TO_ENGINE_FORMAT_VERSION_SETTING_DEPRECATED
Language=English
%1 (%2) %3The database format feature version %5 could not be used due to the current database format %6, controlled by the parameter %4.
.

MessageId=643
SymbolicName=DATABASE_HAS_OUT_OF_DATE_INDEX
Language=English
%1 (%2) %3 Out of date NLS sort version detected on the database '%4' for Locale '%5', index sort version: (SortId=%6, Version=%7), current sort version: (SortId=%8, Version=%9).
.

MessageId=644
SymbolicName=LOG_CHECKPOINT_GETTING_DEEP_ID
Language=English
%1 (%2) %3A session has an outstanding transaction causing the checkpoint depth (of %4 logs) to exceed a quarter of the JET_paramCheckpointTooDeep setting.%n
%n
Details:%n
Transaction Context Info: %5%n
Transaction Start Times: %6%n
.

MessageId=645
SymbolicName=TRAILER_PAGE_MISSING_OR_CORRUPT_ID
Language=English
%1 (%2) %3Attempted to use trailer page to patch header in database '%4' but it was found to be corrupt. This may indicate an incomplete backup and a missing trailer page.
.

MessageId=646
SymbolicName=RBS_CORRUPT_ID
Language=English
%1 (%2) %3The revert snapshot file at "%4" is corrupt with reason '%5' at offset %6, segment %7. The expected checksum was %8 and the actual checksum was %9.
.

MessageId=647
SymbolicName=EXTENT_PAGE_COUNT_CACHE_IN_USE_ID
Language=English
%1 (%2) %3The Extent Page Count Cache is enabled in database %4 for reason %5.
.

MessageId=648
SymbolicName=EXTENT_PAGE_COUNT_CACHE_NOT_IN_USE_ID
Language=English
%1 (%2) %3The Extent Page Count Cache is disabled in database %4 for reason %5.
.

MessageId=649
SymbolicName=EXTENT_PAGE_COUNT_CACHE_CREATED_ID
Language=English
%1 (%2) %3The Extent Page Count Cache table was created in database %4 for reason %5.
.

MessageId=650
SymbolicName=EXTENT_PAGE_COUNT_CACHE_DELETED_ID
Language=English
%1 (%2) %3The Extent Page Count Cache table was deleted in database %4 for reason %5.
.

MessageId=651
SymbolicName=EXTENT_PAGE_COUNT_CACHE_TRACK_ON_CREATE_ENABLED_ID
Language=English
%1 (%2) %3Newly created tables will be tracked immediately by the Extent Page Count Cache.
.

MessageId=652
SymbolicName=EXTENT_PAGE_COUNT_CACHE_EXTENSIVE_VALIDATION_ENABLED_ID
Language=English
%1 (%2) %3Extra validation of the Extent Page Count Cache table is enabled.  This is resource intensive and will adversely affect performance.
.

MessageId=653
SymbolicName=EXTENT_PAGE_COUNT_CACHE_OPERATION_FAILED_ID
Language=English
%1 (%2) %3An operation in the Extent Page Count Cache table failed.%n
Database: %4%n
Operation: %5%n
Note: %6%n
Err: %7%n
ObjidFDP: %8%n
PgnoFDP: %9%n
.

MessageId=654
SymbolicName=EXTENT_PAGE_COUNT_CACHE_PREPARE_FAILED_ID
Language=English
%1 (%2) %3Preparing a row in the Extent Page Count Cache table failed.  The cached value may be incorrect.%n
Database: %4%n
Objid: %5%n
PgnoFDP: %6%n
Err: %7%n
.

MessageId=655
SymbolicName=EXTENT_PAGE_COUNT_CACHE_EXTENSIVE_VALIDATION_FAILED_ID
Language=English
%1 (%2) %3Extra validation of the Extent Page Count Cache table failed.%n
Database: %4%n
Objid: %5%n
PgnoFDP: %6%n
Counted CpgOE: %7%n
Counted CpgAE: %8%n
Cached CpgOE: %9%n
Cached CpgAE: %10%n
.

MessageId=656
SymbolicName=BEGIN_TRANSACTION_FOR_EXTENT_PAGE_COUNT_CACHE_PREPARE_FAILED_ID
Language=English
%1 (%2) %3Beginning a transaction to prepare a row in the Extent Page Count Cache table failed.  The cached value is now incorrect.%n
Database: %4%n
Objid: %5%n
PgnoFDP: %6%n
Err: %7%n
.


;// !!! ARE YOU SURE you're adding this in the right place !!! ???



;///////////////////////////////////////////////////////////
;//	Database Maintenance Tasks and Status
;///////////////////////////////////////////////////////////

MessageId=700
SymbolicName=OLD_BEGIN_FULL_PASS_ID
Language=English
%1 (%2) %3Online defragmentation is beginning a full pass on database '%4'.
.

MessageId=701
SymbolicName=OLD_COMPLETE_FULL_PASS_ID
Language=English
%1 (%2) %3Online defragmentation has completed a full pass on database '%4', freeing %5 pages. This pass started on %6 and ran for a total of %7 seconds, requiring %8 invocations over %9 days. Since the database was created it has been fully defragmented %10 times.
.

MessageId=702
SymbolicName=OLD_RESUME_PASS_ID
Language=English
%1 (%2) %3Online defragmentation is resuming its pass on database '%4'. This pass started on %5 and has been running for %6 days.
.

MessageId=703
SymbolicName=OLD_COMPLETE_RESUMED_PASS_ID
Language=English
%1 (%2) %3Online defragmentation has completed the resumed pass on database '%4', freeing %5 pages. This pass started on %6 and ran for a total of %7 seconds, requiring %8 invocations over %9 days. Since the database was created it has been fully defragmented %10 times.
.

MessageId=704
SymbolicName=OLD_INTERRUPTED_PASS_ID
Language=English
%1 (%2) %3Online defragmentation of database '%4' was interrupted and terminated. The next time online defragmentation is started on this database, it will resume from the point of interruption.
.

MessageId=705
SymbolicName=OLD_ERROR_ID
Language=English
%1 (%2) %3Online defragmentation of database '%4' terminated prematurely after encountering unexpected error %5. The next time online defragmentation is started on this database, it will resume from the point of interruption.
.

MessageId=706
SymbolicName=DATABASE_ZEROING_STARTED_ID
Language=English
%1 (%2) %3Online zeroing of database %4 started
.

MessageId=707
SymbolicName=DATABASE_ZEROING_STOPPED_ID
Language=English
%1 (%2) %3Online zeroing of database %4 finished after %5 seconds with err %6%n
%7 pages%n
%8 blank pages%n
%9 pages unchanged since last zero%n
%10 unused pages zeroed%n
%11 used pages seen%n
%12 deleted records zeroed%n
%13 unreferenced data chunks zeroed
.

MessageId=708
SymbolicName=OLDSLV_BEGIN_FULL_PASS_ID
Language=English
%1 (%2) %3Online defragmentation is beginning a full pass on streaming file '%4'.
.

MessageId=709
SymbolicName=OLDSLV_COMPLETE_FULL_PASS_ID
Language=English
%1 (%2) %3Online defragmentation has completed a full pass on streaming file '%4'.
.

MessageId=710
SymbolicName=OLDSLV_SHRANK_DATABASE_ID
Language=English
%1 (%2) %3Online defragmentation of streaming file '%4' has shrunk the file by %5 bytes.
.

MessageId=711
SymbolicName=OLDSLV_ERROR_ID
Language=English
%1 (%2) %3Online defragmentation of streaming file '%4' terminated prematurely after encountering unexpected error %5.
.

MessageId=712
SymbolicName=DATABASE_SLV_ZEROING_STARTED_ID
Language=English
%1 (%2) %3Online zeroing of streaming file '%4' started.
.

MessageId=713
SymbolicName=DATABASE_SLV_ZEROING_STOPPED_ID
Language=English
%1 (%2) %3Online zeroing of streaming file '%4' finished after %5 seconds with err %6%n
%7 pages%n
%8 unused pages zeroed%n
.

MessageId=714
SymbolicName=OLDSLV_STOP_ID
Language=English
%1 (%2) %3Online defragmentation has successfully been stopped on streaming file '%4'.
.

MessageId=715
SymbolicName=OLD_CLEAN_BUFFER_LOW_PAUSE_ID
Language=English
%1 (%2) %3Online defragmentation has been paused one or more times in the last 60 minutes for the following databases: '%4'.  The ESE Database Cache is not large enough to simultaneously run online defragmentation against the listed databases.  Action: Stagger the online maintenance time windows for the listed databases or increase the amount of physical RAM in the server.
.

MessageId=716
SymbolicName=OLD_CLEAN_BUFFER_NORMAL_RESUME_ID
Language=English
%1 (%2) %3Online defragmentation has resumed one or more times in the last 60 minutes for the following databases: "%4'.
.

MessageId=717
SymbolicName=SCAN_CHECKSUM_START_PASS_ID
Language=English
%1 (%2) %3Online Maintenance is starting Database Checksumming background task for database '%4'.
.

MessageId=718
SymbolicName=SCAN_ZERO_START_PASS_ID_DEPRECATED
Language=English
%1 (%2) %3Online Maintenance is starting Database Page Zeroing background task for database '%4'.
.

MessageId=719
SymbolicName=SCAN_CHECKSUM_RESUME_PASS_ID
Language=English
%1 (%2) %3Online Maintenance is resuming Database Checksumming background task for database '%4'. This pass started on %5 and has been running for %6 days.
.

MessageId=720
SymbolicName=SCAN_ZERO_RESUME_PASS_ID_DEPRECATED
Language=English
%1 (%2) %3Online Maintenance is resuming Database Zeroing background task for database '%4'. This pass started on %5 and has been running for %6 days.
.

MessageId=721
SymbolicName=SCAN_CHECKSUM_COMPLETE_PASS_ID_DEPRECATED
Language=English
%1 (%2) %3Online Maintenance Database Checksumming background task has completed for database '%4'. This pass started on %5 and ran for a total of %6 seconds, requiring %7 invocations over %8 days. Operation summary:
%n
%9 pages seen%n
%10 bad checksums%n
%11 uninitialized pages
.

MessageId=722
SymbolicName=SCAN_ZERO_COMPLETE_PASS_ID_DEPRECATED
Language=English
%1 (%2) %3Online Maintenance Database Zeroing background task has completed for database '%4'. This pass started on %5 and ran for a total of %6 seconds, requiring %7 invocations over %8 days. Operation summary:
%n
%9 pages seen%n
%10 bad checksums%n
%11 uninitialized pages%n
%12 pages unchanged since last zero%n
%13 unused pages zeroed%n
%14 used pages seen%n
%15 deleted records zeroed%n
%16 unreferenced data chunks zeroed
.

MessageId=723
SymbolicName=SCAN_CHECKSUM_ERROR_ID_DEPRECATED
Language=English
%1 (%2) %3Online Maintenance Database Checksumming background task for database '%4' encountered error %5 after %6 seconds.
.

MessageId=724
SymbolicName=SCAN_ZERO_ERROR_ID_DEPRECATED
Language=English
%1 (%2) %3Online Maintenance Database Zeroing background task for database '%4' encountered error %5 after %6 seconds.
.

MessageId=725
SymbolicName=SCAN_CHECKSUM_INTERRUPTED_PASS_ID_DEPRECATED
Language=English
%1 (%2) %3Online Maintenance Database Checksumming background task for database '%4' was interrupted and terminated. The next time Online Maintenance Database Checksumming is started on this database, it will resume from the point of interruption.
.

MessageId=726
SymbolicName=SCAN_ZERO_INTERRUPTED_PASS_ID_DEPRECATED
Language=English
%1 (%2) %3Online Maintenance Database Zeroing background task for database '%4' was interrupted and terminated. The next time Online Maintenance Database Checksumming is started on this database, it will resume from the point of interruption.
.

MessageId=727
SymbolicName=SCAN_PAGE_CHECKSUM_MISMATCH_ID
Language=English
%1 (%2) %3The database page read from the file '%4' at offset %5 (database page %6) failed verification due to a page checksum mismatch. If this condition persists then please restore the database from a previous backup. This problem is likely due to faulty hardware. Please contact your hardware vendor for further assistance diagnosing the problem.
.

MessageId=728
SymbolicName=OLD2_COMPLETE_PASS_ID
Language=English
%1 (%2) %3
Online defragmentation has defragmented index '%6' of table '%5' in database '%4'. This pass started on %7.%n
%8 pages visited%n
%9 pages freed%n
%10 partial merges%n
%11 pages moved
.

MessageId=729
SymbolicName=SCAN_CLEAN_BUFFER_LOW_PAUSE_ID
Language=English
%1 (%2) %3Online maintenance database zeroing has been paused one or more times in the last 60 minutes for the following databases: '%4'. The ESE database cache is not large enough to simultaneously run online page zeroing against the listed databases. Action: Stagger the online maintenance time windows for the listed databases or increase the amount of physical RAM in the server.
.

MessageId=730
SymbolicName=SCAN_CLEAN_BUFFER_NORMAL_RESUME_ID
Language=English
%1 (%2) %3Online maintenance database zeroing has resumed one or more times in the last 60 minutes for the following databases: '%4'.
.

MessageId=732
SymbolicName=SCAN_CHECKSUM_ONTIME_PASS_ID
Language=English
%1 (%2) %3Online Maintenance Database Checksumming background task has completed for database '%4'. This pass started on %5 and ran for a total of %6 seconds (over %7 days) on %8 pages.
%n
Last Runtime Scan Stats: %9
.

MessageId=733
SymbolicName=SCAN_CHECKSUM_LATE_PASS_ID
Language=English
%1 (%2) %3Online Maintenance Database Checksumming background task is not finishing on time for database '%4'. This pass started on %5 and has been running for %6 seconds (over %7 days) so far.
.

MessageId=734
SymbolicName=DB_MAINTENANCE_START_FAILURE_ID
Language=English
%1 (%2) %3Database Maintenance background task for database '%4' failed to start with error %5.  
.

MessageId=735
SymbolicName=DB_MAINTENANCE_LATE_PASS_ID
Language=English
%1 (%2) %3Database Maintenance has completed a full pass on database '%4'. This pass started on %5 and ran for a total of %6 seconds (%7 days).  This database maintenance task exceeded the %8 day maintenance completion threshold.  One or more of the following actions should be taken:  increase the IO performance/throughput of the volume hosting the database, reduce the database size, and/or reduce non-database maintenance IO.
%n
%9 pages seen%n
Average free space on Record pages: %10 (%11 pages scanned)%n
Average free space on LV pages: %12 (%13 pages scanned)%n
Last Runtime Scan Stats: %10%n
.

MessageId=736
SymbolicName=DB_MAINTENANCE_START_PASS_ID
Language=English
%1 (%2) %3Database Maintenance is starting for database '%4'.
.

MessageId=737
SymbolicName=DB_MAINTENANCE_COMPLETE_PASS_ID
Language=English
%1 (%2) %3Database Maintenance has completed a full pass on database '%4'. This pass started on %5 and ran for a total of %6 seconds.
%n
%7 pages seen%n
Average free space on Record pages: %9 (%10 pages scanned)%n
Average free space on LV pages: %11 (%12 pages scanned)%n
Last Runtime Scan Stats: %8%n
.

MessageId=738
SymbolicName=DB_MAINTENANCE_RESUME_PASS_ID
Language=English
%1 (%2) %3Database Maintenance is resuming for database '%4', starting from page %5. This pass started on %6 and has been running for %7 days.
.

MessageId=739
SymbolicName=NTFS_FILE_ATTRIBUTE_MAX_SIZE_EXCEEDED
Language=English
%1 (%2) %3The NTFS file attributes size for database '%4' is %5 bytes, which exceeds the threshold of %6 bytes. The database file must be reseeded or restored from a copy or backup to prevent the database file from being unable to grow because of  a file system limitation. 
.

MessageId=740
SymbolicName=DB_TRIM_TASK_STARTED
Language=English
%1 (%2) %3The periodic database file trimming operation started.
.

MessageId=741
SymbolicName=DB_TRIM_TASK_SUCCEEDED
Language=English
%1 (%2) %3The periodic database file trimming operation finished successfully and trimmed the database file by %4 pages.
.

MessageId=742
SymbolicName=DB_TRIM_TASK_NO_TRIM
Language=English
%1 (%2) %3The periodic database file trimming operation was not able to trim the database file because there is not enough internal free space available.
.

MessageId=743
SymbolicName=DB_TRIM_TASK_FAILED
Language=English
%1 (%2) %3The periodic database file trimming operation was not able to trim the database file. Result %4.
.

MessageId=744
SymbolicName=DB_MAINTENANCE_NOTIFY_RUNNING_ID
Language=English
%1 (%2) %3Database Maintenance is running on database '%4'. This pass started on %5 and has been running for %6 hours.
%n
%7 pages seen%n
.

MessageId=745
SymbolicName=SCAN_CHECKSUM_CRITICAL_ABORT_ID
Language=English
%1 (%2) %3Online Maintenance Database Checksumming background task is aborted for database '%4' at page %5, due to client request (typically because of critical redo activity must be done quickly).%n
The last properly completed checksumming task was on %6.%n
%n
Last Runtime Scan Stats: %7%n
.

MessageId=746
SymbolicName=SCAN_CHECKSUM_INCOMPLETE_PASS_FINISHED_ID
Language=English
%1 (%2) %3Online Maintenance Database Checksumming background task finished an interrupted and incomplete scan for database '%4'. Typical interruptions are due to client request (typically because of critical redo activity must be done quickly).%n
The last fully completed checksumming scan was on %5.
.

MessageId=747
SymbolicName=SCAN_CHECKSUM_RESUME_SKIPPED_PAGES_ID
Language=English
%1 (%2) %3Online Maintenance is resuming Database Checksumming background task for database '%4', and in doing so has skipped %5 pages (%6%%).  prev stop: %7, this start: %8.%n
.

MessageId=748
SymbolicName=DB_SHRINK_SUCCEEDED_ID
Language=English
%1 (%2) %3The database file shrinkage operation completed successfully on database '%4'.%n
Initial file size: %7 bytes (%8 page(s)).%n
Final file size: %9 bytes (%10 page(s)).%n
Initial owned space: %11 bytes (%12 page(s)).%n
Final owned space: %13 bytes (%14 page(s)).%n
Data moved: %15 bytes (%16 page(s)).%n
Pages shelved: %24 page(s).%n
Pages unleaked: %25 page(s).%n
Return code: %17%n
Stop reason: %18%n
Total time: %5 minute(s) and %6 second(s).%n
Pct. time in extent maintenance: %19%%%n
Pct. time in file truncation: %20%%%n
Pct. time in page categorization: %21%%%n
Pct. time in data move: %22%%%n
Pct. remaining time: %23%%%n
.

MessageId=749
SymbolicName=DB_SHRINK_FAILED_ID
Language=English
%1 (%2) %3The database file shrinkage operation failed on database '%4'.%n
Initial file size: %7 bytes (%8 page(s)).%n
Final file size: %9 bytes (%10 page(s)).%n
Initial owned space: %11 bytes (%12 page(s)).%n
Final owned space: %13 bytes (%14 page(s)).%n
Data moved: %15 bytes (%16 page(s)).%n
Pages shelved: %24 page(s).%n
Pages unleaked: %25 page(s).%n
Error code: %17%n
Stop reason: %18%n
Total time: %5 minute(s) and %6 second(s).%n
Pct. time in extent maintenance: %19%%%n
Pct. time in file truncation: %20%%%n
Pct. time in page categorization: %21%%%n
Pct. time in data move: %22%%%n
Pct. remaining time: %23%%%n
.

MessageId=750
SymbolicName=DB_LEAK_RECLAIMER_SUCCEEDED_ID
Language=English
%1 (%2) %3The space leakage reclaimer completed successfully on database '%4'.%n
Stop reason: %5%n
Total time: %6 minute(s) and %7 second(s).%n
Return code: %8%n
Initial owned space: %9 bytes (%10 page(s)).%n
Final owned space: %11 bytes (%12 page(s)).%n
Leaked pages reclaimed: %13 page(s).%n
Reclaimed page number range: %14 - %15%n
Shelved page count: %16 page(s).%n
Shelved page number range: %17 - %18%n
.

MessageId=751
SymbolicName=DB_LEAK_RECLAIMER_FAILED_ID
Language=English
%1 (%2) %3The space leakage reclaimer failed on database '%4'.%n
Stop reason: %5%n
Total time: %6 minute(s) and %7 second(s).%n
Return code: %8%n
Initial owned space: %9 bytes (%10 page(s)).%n
Final owned space: %11 bytes (%12 page(s)).%n
Leaked pages reclaimed: %13 page(s).%n
Reclaimed page number range: %14 - %15%n
Shelved page count: %16 page(s).%n
Shelved page number range: %17 - %18%n
.

MessageId=752
SymbolicName=OLD2_TABLE_STATUS
Language=English
%1 (%2) %3There are %4 outstanding tasks in the online defragmentation table for database %5.%n
%6 of those outstanding tasks have not started.%n
.


;// !!! ARE YOU SURE you're adding this in the right place !!! ???



;///////////////////////////////////////////////////////////
;//	System Parameter Validation
;///////////////////////////////////////////////////////////

MessageId=800
SymbolicName=SYS_PARAM_CACHEMIN_CSESS_ERROR_ID
Language=English
%1 (%2) %3System parameter minimum cache size (%4) is less than 4 times the number of sessions (%5).
.

MessageId=801
SymbolicName=SYS_PARAM_CACHEMAX_CACHEMIN_ERROR_ID
Language=English
%1 (%2) %3System parameter maximum cache size (%4) is less than minimum cache size (%5).
.

MessageId=802
SymbolicName=SYS_PARAM_CACHEMAX_STOPFLUSH_ERROR_ID
Language=English
%1 (%2) %3System parameter maximum cache size (%4) is less than stop clean flush threshold (%5).
.

MessageId=803
SymbolicName=SYS_PARAM_STOPFLUSH_STARTFLUSH_ERROR_ID
Language=English
%1 (%2) %3System parameter stop clean flush threshold (%4) is less than start clean flush threshold (%5).
.

MessageId=804
SymbolicName=SYS_PARAM_LOGBUFFER_FILE_ERROR_ID
Language=English
%1 (%2) %3System parameter log buffer size (%4 sectors) is greater than the maximum size of %5 k bytes (logfile size minus reserved space).
.

MessageId=805
SymbolicName=SYS_PARAM_MAXPAGES_PREFER_ID
Language=English
%1 (%2) %3System parameter max version pages (%4) is less than preferred version pages (%5).
.

MessageId=806
SymbolicName=SYS_PARAM_VERPREFERREDPAGE_ID
Language=English
%1 (%2) %3System parameter preferred version pages was changed from %4 to %5 due to physical memory limitation.
.

MessageId=807
SymbolicName=SYS_PARAM_CFCB_PREFER_ID
Language=English
%1 (%2) %3System parameter max open tables (%4) is less than preferred opentables (%5). Please check registry settings.
.

MessageId=808
SymbolicName=SYS_PARAM_VERPREFERREDPAGETOOBIG_ID
Language=English
%1 (%2) %3System parameter preferred version pages (%4) is greater than max version pages (%5). Preferred version pages was changed from %6 to %7.
.

;// !!! ARE YOU SURE you're adding this in the right place !!! ???



;///////////////////////////////////////////////////////////
;//	Not sure what this section is for?
;///////////////////////////////////////////////////////////

MessageId=900
SymbolicName=LOG_COMMIT0_FAIL_ID
Language=English
%1 (%2) %3The database engine failed with error %4 while trying to log the commit of a transaction. To ensure database consistency, the process was terminated. Simply restart the process to force database recovery and return the database to a Clean Shutdown state.
.

MessageId=901
SymbolicName=INTERNAL_TRACE_ID
Language=English
%1 (%2) %3Tag: %6. Internal trace: %4 (%5).
.

MessageId=902
SymbolicName=SESSION_SHARING_VIOLATION_ID
Language=English
%1 (%2) %3The database engine detected multiple threads illegally using the same database session to perform database operations.
%n%tSessionId: %4
%n%tSession-context: %5
%n%tSession-context ThreadId: %6
%n%tCurrent ThreadId: %7
%n%tSession-trace:
%n%8
.

MessageId=903
SymbolicName=SESSION_WRITE_CONFLICT_ID
Language=English
%1 (%2) %3The database engine detected two different cursors of the same session illegally trying to update the same record at the same time.
%n%tSessionId: %4
%n%tThreadId: %5
%n%tDatabase: %6
%n%tTable: %7
%n%tCurrent cursor: %8
%n%tCursor owning original update: %9
%n%tBookmark size (prefix/suffix): %10
%n%tSession-trace:
%n%11
.

MessageId=904
SymbolicName=CORRUPTING_PAGE_ID_DEPRECATED
Language=English
%1 (%2) %3A test API has been used to corrupt page %4 of database '%5'.
.

MessageId=905
SymbolicName=PAGE_PATCHED_ID
Language=English
%1 (%2) %3The database engine repaired corruption on page %4 of database '%5'. Although the corruption has been repaired, the source of the corruption is likely due to faulty hardware and should be investigated.  Please contact your hardware vendor for further assistance diagnosing the problem.
.

MessageId=906
SymbolicName=RESIDENT_CACHE_HAS_FALLEN_TOO_FAR_ID
Language=English
%1 (%2) %3A significant portion of the database buffer cache has been written out to the system paging file. This may result in severe performance degradation.
%nSee help link for complete details of possible causes.
%nPrevious cache residency state: %4% (%5 out of %6 buffers) (%7 seconds ago)
%nCurrent cache residency state:  %8% (%9 out of %10 buffers)
%nCurrent cache size vs. target:  %11% (%12 MBs)
%nPhysical Memory / RAM size:     %13 MBs
.

MessageId=907
SymbolicName=TRANSIENT_IN_MEMORY_CORRUPTION_DETECTED_ID
Language=English
%1 (%2) %3A transient memory corruption was detected.  Please run the Windows Memory Diagnostics Tool and/or investigate hardware issues.
.

MessageId=908
SymbolicName=ENFORCE_FAIL
Language=English
%1 (%2) %3Terminating process due to non-recoverable failure: %4 (%5). Tag: %6.
.

MessageId=909
SymbolicName=ENFORCE_DB_FAIL
Language=English
%1 (%2) %3Terminating process due to non-recoverable failure: %4 (%5) in operation on database '%6'.
.

MessageId=910
SymbolicName=CACHE_SIZE_MAINT_TAKING_TOO_LONG
Language=English
%1 (%2) %3The database cache size maintenance task has taken %4 seconds without completing. This may result in severe performance degradation.
Current cache size is %5 buffers above the configured cache limit (%6 percent of target).
Cache size maintenance evicted %7 buffers, made %8 flush attempts, and successfully flushed %9 buffers. It has run %10 times since maintenance was triggered.
.

MessageId=911
SymbolicName=PAGE_PATCHED_PASSIVE_ID
Language=English
%1 (%2) %3The database engine repaired corruption on pages %4 - %5 of database '%6'. Although the corruption has been repaired, the source of the corruption is likely due to faulty hardware and should be investigated.  Please contact your hardware vendor for further assistance diagnosing the problem.
.

MessageId=912
SymbolicName=RESIDENT_CACHE_IS_RESTORED_ID
Language=English
%1 (%2) %3A portion of the database buffer cache has been restored from the system paging file and is now resident again in memory. Prior to this, a portion of the database buffer cache had been written out to the system paging file resulting in performance degradation.
%nPrevious cache residency state: %4% (%5 out of %6 buffers) (%7 seconds ago)
%nCurrent cache residency state: %8% (%9 out of %10 buffers)
.

MessageId=913
SymbolicName=GLOBAL_SYSTEM_PARAMETER_MISMATCH_ID
Language=English
%1 (%2) %3The parameter %4 was not set to one value by the previous ESE application instance, but set to a different value from the current ESE application. This is a system parameter mismatch, all ESE applications must agree on all global system parameters.
.

MessageId=914
SymbolicName=GLOBAL_SYSTEM_PARAMETER_NOT_SET_PREVIOUSLY_MISMATCH_ID
Language=English
%1 (%2) %3The parameter %4 was not set via the previous ESE application instance, but is set in the current ESE application. This is a system parameter mismatch, all ESE applications must agree on all global system parameters.
.

MessageId=915
SymbolicName=GLOBAL_SYSTEM_PARAMETER_SET_PREVIOUSLY_MISMATCH_ID
Language=English
%1 (%2) %3The parameter %4 was set via the previous ESE application instance, but is not set in the current ESE application. This is a system parameter mismatch, all ESE applications must agree on all global system parameters.
.

MessageId=916
SymbolicName=STAGED_BETA_FEATURE_IN_USE_ID
Language=English
%1 (%2) %3The beta feature %4 is enabled in %5 due to the beta site mode settings %6.
.

;// !!! THIS IS PROBABLY NOT THE RIGHT PLACE!!!  This area is reserved for bad programmer events: Asserts, Enforces, session write conflicts, etc.  Make sure you're adding this in the right place!!!



;///////////////////////////////////////////////////////////
;//	RFS Events
;///////////////////////////////////////////////////////////

MessageId=1000
SymbolicName=RFS2_INIT_ID
Language=English
%1 (%2) %3Resource failure simulation was activated with the following settings:
\t\t%4:\t%5
\t\t%6:\t%7
\t\t%8:\t%9
\t\t%10:\t%11
.

MessageId=1001
SymbolicName=RFS2_PERMITTED_ID
Language=English
%1 (%2) %3Resource failure simulation %4 is permitted.
.

MessageId=1002
SymbolicName=RFS2_DENIED_ID
Language=English
%1 (%2) %3Resource Failure Simulation %4 is denied.
.

MessageId=1003
SymbolicName=RFS2_JET_CALL_ID
Language=English
%1 (%2) %3JET call %4 returned error %5. %6 (%7)
.

MessageId=1004
SymbolicName=RFS2_JET_ERROR_ID
Language=English
%1 (%2) %3JET inline error %4 jumps to label %5. %6 (%7)
.

;//Inproper event placement, moving the event to 531.  Leave this event dead.
;//MessageId=1005
;//SymbolicName=SUSPECTED_BAD_BLOCK_OVERWRITE_ID
;//Language=English
;//%1 (%2) %3The database engine attempted a clean write operation on page %4 of database %5. This was performed in an attempt to correct a previous problem reading from the page.
;//.

;// !!! ARE YOU SURE you're adding this in the right place !!! ???



;///////////////////////////////////////////////////////////
;//	Snapshot Events
;///////////////////////////////////////////////////////////

MessageId=2000
SymbolicName=OS_SNAPSHOT_TRACE_ID
Language=English
%1 (%2) %3Shadow copy function %4() = %5.
.

MessageId=2001
SymbolicName=OS_SNAPSHOT_FREEZE_START_ID
Language=English
%1 (%2) %3Shadow copy instance %4 freeze started.
.

MessageId=2002
SymbolicName=OS_SNAPSHOT_FREEZE_START_ERROR_ID
Language=English
%1 (%2) %3Shadow copy instance %4 encountered error %5 on freeze.
.

MessageId=2003
SymbolicName=OS_SNAPSHOT_FREEZE_STOP_ID
Language=English
%1 (%2) %3Shadow copy instance %4 freeze ended.
.

MessageId=2004
SymbolicName=OS_SNAPSHOT_TIME_OUT_ID
Language=English
%1 (%2) %3Shadow copy instance %4 freeze timed-out (%5 ms).
.

MessageId=2005
SymbolicName=OS_SNAPSHOT_PREPARE_FULL_ID
Language=English
%1 (%2) %3Shadow copy instance %4 starting. This will be a Full shadow copy.
.

MessageId=2006
SymbolicName=OS_SNAPSHOT_END_ID
Language=English
%1 (%2) %3Shadow copy instance %4 completed successfully.
.

MessageId=2007
SymbolicName=OS_SNAPSHOT_ABORT_ID
Language=English
%1 (%2) %3Shadow copy instance %4 aborted.
.

MessageId=2008
SymbolicName=OS_SNAPSHOT_PREPARE_INCREMENTAL_ID
Language=English
%1 (%2) %3Shadow copy instance %4 starting. This will be an Incremental shadow copy.
.

MessageId=2009
SymbolicName=OS_SNAPSHOT_PREPARE_COPY_ID
Language=English
%1 (%2) %3Shadow copy instance %4 starting. This will be a Copy shadow copy.
.

MessageId=2010
SymbolicName=OS_SNAPSHOT_PREPARE_DIFFERENTIAL_ID
Language=English
%1 (%2) %3Shadow copy instance %4 starting. This will be a Differential shadow copy.
.

;// !!! ARE YOU SURE you're adding this in the right place !!! ???



;///////////////////////////////////////////////////////////
;//	Failure tags ...
;///////////////////////////////////////////////////////////

MessageId=3000
SymbolicName=NOOP_FAILURE_TAG_ID
Language=English
.

MessageId=3001
SymbolicName=CONFIGURATION_FAILURE_TAG_ID
Language=English
%1 (%2) %3A configuration error is preventing proper operation of database '%4' ('%5').  The error may occur because of a database misconfiguration, a permission problem, or a storage-related problem.
%n
%nAdditional diagnostic information:  %6 %7
.

MessageId=3002
SymbolicName=REPAIRABLE_FAILURE_TAG_ID
Language=English
%1 (%2) %3A read verification or I/O error is preventing proper operation of database '%4' ('%5'). Review the event logs and other log data to determine if your system is experiencing storage-related problems.%n%nAdditional diagnostic information:  %6 %7
.

MessageId=3003
SymbolicName=SPACE_FAILURE_TAG_ID
Language=English
%1 (%2) %3Lack of free space for the database or log files is preventing proper operation of database '%4' ('%5').  Review the database and log volume's free space to identify the specific problem.
%n
%nAdditional diagnostic information:  %6 %7
.

MessageId=3004
SymbolicName=IOHARD_FAILURE_TAG_ID
Language=English
%1 (%2) %3An I/O error is preventing proper operation of database '%4' ('%5').  Review events logs and other log data to determine if your system is experiencing storage problems.
%n
%nAdditional diagnostic information:  %6 %7
.

MessageId=3005
SymbolicName=SOURCECORRUPTION_FAILURE_TAG_ID
Language=English
%1 (%2) %3A passive copy has detected a corruption in the mounted copy of database '%4' ('%5'). This error might be the result of a storage-related problem.%n%nAdditional diagnostic information:  %6 %7
.

MessageId=3006
SymbolicName=CORRUPTION_FAILURE_TAG_ID
Language=English
%1 (%2) %3Corruption has been detected in database '%4' ('%5').  The error may occur because of human errors, system, or storage problems.
%n
%nAdditional diagnostic information:  %6 %7
.

MessageId=3007
SymbolicName=HARD_FAILURE_TAG_ID
Language=English
%1 (%2) %3Resources or an operating error is preventing proper functioning of database '%4' ('%5').  Review the application and system event logs for failures of the database or its required system components.
%n
%nAdditional diagnostic information:  %6 %7
.

MessageId=3008
SymbolicName=UNRECOVERABLE_FAILURE_TAG_ID
Language=English
%1 (%2) %3A serious error is preventing proper operation of database '%4' ('%5').  Review the application and system event logs for failures of the database or its required system components.
%n
%nAdditional diagnostic information:  %6 %7
.

MessageId=3009
SymbolicName=REMOUNT_FAILURE_TAG_ID
Language=English
%1 (%2) %3Problems are preventing proper operation of database '%4' ('%5').  The failure may be corrected by remounting the database.
%n
%nAdditional diagnostic information:  %6 %7
.

MessageId=3010
SymbolicName=RESEED_FAILURE_TAG_ID
Language=English
%1 (%2) %3In a log shipping environment, a passive copy has detected an error that is preventing proper operation of database '%4' ('%5').  Review the application and system event logs for failures of the database or its required system components.
.

MessageId=3011
SymbolicName=PERFORMANCE_FAILURE_TAG_ID
Language=English
%1 (%2) %3A performance problem is affecting proper operation of database '%4' ('%5').  The error may occur due to misconfiguration, storage problems, or performance problems on the computer.  Review the performance counters and application event logs associated with the database, its storage, and the computer to identify the specific problems.
%n
%nAdditional diagnostic information:  %6 %7
.

MessageId=3012
SymbolicName=MOVELOAD_FAILURE_TAG_ID
Language=English
%1 (%2) %3An unusually large amount of normal load is preventing proper operation of database '%4' ('%5').  The load on this database should be reduced to restore proper operation.
%n
%nAdditional diagnostic information:  %6 %7
.

MessageId=3013
SymbolicName=MEMORY_FAILURE_TAG_ID
Language=English
%1 (%2) %3The system is experiencing memory allocation failures that are preventing proper operation of database '%4' ('%5').  The error could occur due to a mis-configuration or excessive memory consumption on the machine.
%n
%nAdditional diagnostic information:  %6 %7
.

MessageId=3014
SymbolicName=CATALOGRESEED_FAILURE_TAG_ID
Language=English
.

MessageId=3071
SymbolicName=RBSROLLREQUIRED_FAILURE_TAG_ID
Language=English
.

MessageId=3072
SymbolicName=MAX_FAILURE_TAG_ID
Language=English
.

;///////////////////////////////////////////////////////////
;//	Block Cache Events
;///////////////////////////////////////////////////////////

MessageId=4000
SymbolicName=BLOCK_CACHE_CACHING_FILE_ID_MISMATCH_ID
Language=English
%1 (%2) %3The caching file with the requested physical ids could not be found.%n
%n
Volume id=%4%n
File id=%5%n
Unique id=%6%n
.

MessageId=4001
SymbolicName=BLOCK_CACHE_CACHING_FILE_OPEN_FAILURE_ID
Language=English
%1 (%2) %3The caching file at '%4' could not be opened due to error %5.  Caching cannot be performed for files backed by this cache.%n
.

MessageId=4002
SymbolicName=BLOCK_CACHE_FILE_ID_MISMATCH_ID
Language=English
%1 (%2) %3The file '%4' has a different file id than expected.%n
%n
Expected Volume id=%5%n
Expected File id=%6%n
%n
Actual Volume id=%7%n
Actual File id=%8%n
.

MessageId=4003
SymbolicName=BLOCK_CACHE_CACHED_FILE_ID_MISMATCH_ID
Language=English
%1 (%2) %3The cached file with the requested physical ids could not be found.%n
%n
Volume id=%4%n
File id=%5%n
File serial=%6%n
.

;///////////////////////////////////////////////////////////
;//	RBS Events
;///////////////////////////////////////////////////////////

MessageId=5000
SymbolicName=RBS_CREATEORLOAD_SUCCESS_ID
Language=English
%1 (%2) %3The revert snapshot file at "%4" was "%5" successfully.
.

MessageId=5001
SymbolicName=RBS_CREATEORLOAD_FAILED_ID
Language=English
%1 (%2) %3The revert snapshot file at "%4" failed to be "%5" with error "%6".
.

MessageId=5002
SymbolicName=RBSCLEANER_REMOVEDRBS_ID
Language=English
%1 (%2) %3The revert snapshot directory "%4" was removed by RBS cleaner. Reason: "%5".
.

MessageId=5003
SymbolicName=RBSREVERT_EXECUTE_SUCCESS_ID
Language=English
%1 (%2) %3Databases were reverted from "%4" to "%5" successfully.
.

MessageId=5004
SymbolicName=RBSREVERT_EXECUTE_FAILED_ID
Language=English
%1 (%2) %3Databases were failed to be reverted from "%4" to "%5" with error "%6".
.

MessageId=5005
SymbolicName=RBS_CREATE_SKIPPED_ID
Language=English
%1 (%2) %3The creation of revert snapshot file at "%4" was skipped due to "%5". Load failed with error "%6".
.

MessageId=5006
SymbolicName=RBS_SPACE_GROWTH_ID
Language=English
%1 (%2) %3The revert snapshot file "%4" created on "%5" has grown by "%6 bytes" since "%7". Number of logs generated during this period was "%8".
.

MessageId=5007
SymbolicName=RBS_INVALIDATED_ID
Language=English
%1 (%2) %3The revert snapshot file at "%4" was invalidated due to "%5".
.

;////////////////////////////////////////////////////////////////////////
;// DO NOT ADD HERE!  You're in the wrong Place. (well most likely)
;////////////////////////////////////////////////////////////////////////
