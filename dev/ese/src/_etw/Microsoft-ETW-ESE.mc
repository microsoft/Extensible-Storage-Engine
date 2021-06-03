<?xml version='1.0' encoding='utf-8' standalone='yes'?>

<!--
Copyright (c) Microsoft Corporation.
Licensed under the MIT License.
-->
<!-- For adding new ETW traces, see the list of steps in oseventtrace.cxx -->
ESE_PRE_GEN_BASE_FILE: This file is not meant to be compiled directly, it should 
ESE_PRE_GEN_BASE_FILE:   be pre-processed with eseetw.pl.

<assembly
    xmlns="urn:schemas-microsoft-com:asm.v3"
    xmlns:xsd="http://www.w3.org/2001/XMLSchema"
    xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
    manifestVersion="1.0"
    >
  <assemblyIdentity
      buildType="$(build.buildType)"
      language="neutral"
      name="xxxESE_ETW_ASM_IDxxx"
      processorArchitecture="$(build.arch)"
      publicKeyToken="$(Build.WindowsPublicKeyToken)"
      version="$(build.version)"
      versionScope="nonSxS"
      />
  <dependency
      buildFilter="not build.isWow"
      discoverable="false"
      optional="false"
      resourceType="Resources"
      >
    <dependentAssembly>
      <assemblyIdentity
          buildType="$(build.buildType)"
          language="*"
          name="xxxESE_ETW_ASM_IDxxx.Resources"
          processorArchitecture="$(build.arch)"
          publicKeyToken="$(Build.WindowsPublicKeyToken)"
          version="$(build.version)"
          />
    </dependentAssembly>
  </dependency>
  <file
      destinationPath="$(runtime.system32)\"
      importPath="$(build.nttree)\"
      name="ETWESEProviderResources.dll"
      sourceName="ETWESEProviderResources.dll"
      sourcePath=".\"
      >
    <signatureInfo>
      <signatureDescriptor pageHash="true"/>
    </signatureInfo>
  </file>

  <instrumentation
      xmlns:trace="http://schemas.microsoft.com/win/2004/08/events/trace"
      xmlns:win="http://manifests.microsoft.com/win/2004/08/windows/events"
      xsi:schemaLocation="http://schemas.microsoft.com/win/2004/08/events eventman.xsd"
      buildFilter="not build.isWow"
      >

    <events xmlns="http://schemas.microsoft.com/win/2004/08/events">

      <!--To avoid spoofing, add provider security settings under
          HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\WMI\security

          ** Never log any data which should be protected for security or privacy purposes.
          -->
      <provider
          guid="{xxxESE_ETW_PROV_GUIDxxx}"
          messageFileName="%systemroot%\system32\ETWESEProviderResources.dll"
          name="xxxESE_ETW_PROV_NAMExxx"
          resourceFileName="%systemroot%\system32\ETWESEProviderResources.dll"
          symbol="xxxESE_ETW_PROV_SYMBOLxxx"
          >

        <channels>
          <channel
              chid="OpChannel" 
              name="xxxESE_ETW_CHANNEL_NAMExxx/Operational" 
              type="Operational" 
              symbol="SymOpChannel"
              enabled="xxxESE_ETW_CHANNEL_ENABLEDxxx"
              isolation="Application">
              <logging>
                  <maxSize>xxxESE_ETW_CHANNEL_SIZExxx</maxSize>
              </logging>
          </channel>  
          <channel
              chid="IODiagnoseChannel" 
              name="xxxESE_ETW_CHANNEL_NAMExxx/IODiagnose" 
              type="Debug" 
              symbol="SymIODiagnoseChannel" 
              isolation="Application">
              <logging>
                  <maxSize>20971520</maxSize>
              </logging>
          </channel>
        </channels>

        <!-- ==========================================================================================================
                      KEYWORDS
                    (in instrumentation\events\provider\keywords)
             ==========================================================================================================
              Keyword masks are 64-bit, of which the lower 48 bits may be used.
              For more information on keyword definitions, please see
              http://msdn.microsoft.com/en-us/library/aa382786(VS.85).aspx
          -->
        <keywords>
          <!-- Keywords 1 and 2 are defined by WinPhone best practices. -->
          <keyword
              mask="0x0000000000000001"
              message="$(string.Keyword.Error)"
              name="Error"
              />
          <keyword
              mask="0x0000000000000002"
              message="$(string.Keyword.Performance)"
              name="Performance"

              />
          <!-- Other keywords are provider-specific. -->
          <keyword
              mask="0x0000000000000004"
              message="$(string.Keyword.Trace)"
              name="Trace"
              />
          <keyword
              mask="0x0000000000000008"
              message="$(string.Keyword.Transaction)"
              name="Transaction"
              />
          <keyword
              mask="0x0000000000000010"
              message="$(string.Keyword.Space)"
              name="Space"
              />
          <keyword
              mask="0x0000000000000020"
              message="$(string.Keyword.BF)"
              name="BF"
              />
          <keyword
              mask="0x0000000000000040"
              message="$(string.Keyword.IO)"
              name="IO"
              />
          <keyword
              mask="0x0000000000000080"
              message="$(string.Keyword.LOG)"
              name="LOG"
              />
          <keyword
              mask="0x0000000000000100"
              message="$(string.Keyword.Task)"
              name="Task"
              />
          <keyword
              mask="0x0000000000000200"
              message="$(string.Keyword.Test)"
              name="Test"
              />
          <keyword
              mask="0x0000000000000400"
              message="$(string.Keyword.BFRESMGR)"
              name="BFRESMGR"
              />
          <keyword
              mask="0x0000000000000800"
              message="$(string.Keyword.StationId)"
              name="StationId"
              />
          <keyword
              mask="0x0000000000001000"
              message="$(string.Keyword.JETTraceTag)"
              name="JETTraceTag"
              />
          <keyword
              mask="0x0000000000002000"
              message="$(string.Keyword.StallLatencies)"
              name="StallLatencies"
              />
          <keyword
              mask="0x0000000000004000"
              message="$(string.Keyword.DataWorkingSet)"
              name="DataWorkingSet"
              />
          <keyword
              mask="0x0000000000008000"
              message="$(string.Keyword.IOEX)"
              name="IOEX"
              />
          <keyword
              mask="0x0000000000010000"
              message="$(string.Keyword.IOSESS)"
              name="IOSESS"
              />
          <keyword
              mask="0x0000000000020000"
              message="$(string.Keyword.SubstrateTelemetry)"
              name="SubstrateTelemetry"
              />

          <!-- Temporary trace for collecting data for a compression experiment -->
          <keyword
              mask="0x0000000100000000"
              message="$(string.Keyword.CompressExp)"
              name="CompressExp"
              />
        </keywords>

        <!-- ==========================================================================================================
                      TASKS
                    (in instrumentation\events\provider\tasks)
             ==========================================================================================================
          -->
        <tasks>
          <!--ESE_ETW_AUTOGEN_TASK_LIST_BEGIN-->
            <!-- DO NOT EDIT = THESE TASKS AUTO-GEN (from EseEtwEventsPregen.txt) -->
          <task
              name="ESE_TraceBaseId_Trace"
              value="1"
              />
          <task
              name="ESE_BF_Trace"
              value="2"
              />
          <task
              name="ESE_Block_Trace"
              value="3"
              />
          <task
              name="ESE_CacheNewPage_Trace"
              value="4"
              />
          <task
              name="ESE_CacheReadPage_Trace"
              value="5"
              />
          <task
              name="ESE_CachePrereadPage_Trace"
              value="6"
              />
          <task
              name="ESE_CacheWritePage_Trace"
              value="7"
              />
          <task
              name="ESE_CacheEvictPage_Trace"
              value="8"
              />
          <task
              name="ESE_CacheRequestPage_Trace"
              value="9"
              />
          <task
              name="ESE_LatchPageDeprecated_Trace"
              value="10"
              />
          <task
              name="ESE_CacheDirtyPage_Trace"
              value="11"
              />
          <task
              name="ESE_TransactionBegin_Trace"
              value="12"
              />
          <task
              name="ESE_TransactionCommit_Trace"
              value="13"
              />
          <task
              name="ESE_TransactionRollback_Trace"
              value="14"
              />
          <task
              name="ESE_SpaceAllocExt_Trace"
              value="15"
              />
          <task
              name="ESE_SpaceFreeExt_Trace"
              value="16"
              />
          <task
              name="ESE_SpaceAllocPage_Trace"
              value="17"
              />
          <task
              name="ESE_SpaceFreePage_Trace"
              value="18"
              />
          <task
              name="ESE_IorunEnqueue_Trace"
              value="19"
              />
          <task
              name="ESE_IorunDequeue_Trace"
              value="20"
              />
          <task
              name="ESE_IOCompletion_Trace"
              value="21"
              />
          <task
              name="ESE_LogStall_Trace"
              value="22"
              />
          <task
              name="ESE_LogWrite_Trace"
              value="23"
              />
          <task
              name="ESE_EventLogInfo_Trace"
              value="24"
              />
          <task
              name="ESE_EventLogWarn_Trace"
              value="25"
              />
          <task
              name="ESE_EventLogError_Trace"
              value="26"
              />
          <task
              name="ESE_TimerQueueScheduleDeprecated_Trace"
              value="27"
              />
          <task
              name="ESE_TimerQueueRunDeprecated_Trace"
              value="28"
              />
          <task
              name="ESE_TimerQueueCancelDeprecated_Trace"
              value="29"
              />
          <task
              name="ESE_TimerTaskSchedule_Trace"
              value="30"
              />
          <task
              name="ESE_TimerTaskRun_Trace"
              value="31"
              />
          <task
              name="ESE_TimerTaskCancel_Trace"
              value="32"
              />
          <task
              name="ESE_TaskManagerPost_Trace"
              value="33"
              />
          <task
              name="ESE_TaskManagerRun_Trace"
              value="34"
              />
          <task
              name="ESE_GPTaskManagerPost_Trace"
              value="35"
              />
          <task
              name="ESE_GPTaskManagerRun_Trace"
              value="36"
              />
          <task
              name="ESE_TestMarker_Trace"
              value="37"
              />
          <task
              name="ESE_ThreadCreate_Trace"
              value="38"
              />
          <task
              name="ESE_ThreadStart_Trace"
              value="39"
              />
          <task
              name="ESE_CacheVersionPage_Trace"
              value="40"
              />
          <task
              name="ESE_CacheVersionCopyPage_Trace"
              value="41"
              />
          <task
              name="ESE_CacheResize_Trace"
              value="42"
              />
          <task
              name="ESE_CacheLimitResize_Trace"
              value="43"
              />
          <task
              name="ESE_CacheScavengeProgress_Trace"
              value="44"
              />
          <task
              name="ESE_ApiCall_Trace"
              value="45"
              />
          <task
              name="ESE_ResMgrInit_Trace"
              value="46"
              />
          <task
              name="ESE_ResMgrTerm_Trace"
              value="47"
              />
          <task
              name="ESE_CacheCachePage_Trace"
              value="48"
              />
          <task
              name="ESE_MarkPageAsSuperCold_Trace"
              value="49"
              />
          <task
              name="ESE_CacheMissLatency_Trace"
              value="50"
              />
          <task
              name="ESE_BTreePrereadPageRequest_Trace"
              value="51"
              />
          <task
              name="ESE_DiskFlushFileBuffers_Trace"
              value="52"
              />
          <task
              name="ESE_DiskFlushFileBuffersBegin_Trace"
              value="53"
              />
          <task
              name="ESE_CacheFirstDirtyPage_Trace"
              value="54"
              />
          <task
              name="ESE_SysStationId_Trace"
              value="55"
              />
          <task
              name="ESE_InstStationId_Trace"
              value="56"
              />
          <task
              name="ESE_FmpStationId_Trace"
              value="57"
              />
          <task
              name="ESE_DiskStationId_Trace"
              value="58"
              />
          <task
              name="ESE_FileStationId_Trace"
              value="59"
              />
          <task
              name="ESE_IsamDbfilehdrInfo_Trace"
              value="60"
              />
          <task
              name="ESE_DiskOsDiskCacheInfo_Trace"
              value="61"
              />
          <task
              name="ESE_DiskOsStorageWriteCacheProp_Trace"
              value="62"
              />
          <task
              name="ESE_DiskOsDeviceSeekPenaltyDesc_Trace"
              value="63"
              />
          <task
              name="ESE_DirtyPage2Deprecated_Trace"
              value="64"
              />
          <task
              name="ESE_IOCompletion2_Trace"
              value="65"
              />
          <task
              name="ESE_FCBPurgeFailure_Trace"
              value="66"
              />
          <task
              name="ESE_IOLatencySpikeNotice_Trace"
              value="67"
              />
          <task
              name="ESE_IOCompletion2Sess_Trace"
              value="68"
              />
          <task
              name="ESE_IOIssueThreadPost_Trace"
              value="69"
              />
          <task
              name="ESE_IOIssueThreadPosted_Trace"
              value="70"
              />
          <task
              name="ESE_IOThreadIssueStart_Trace"
              value="71"
              />
          <task
              name="ESE_IOThreadIssuedDisk_Trace"
              value="72"
              />
          <task
              name="ESE_IOThreadIssueProcessedIO_Trace"
              value="73"
              />
          <task
              name="ESE_IOIoreqCompletion_Trace"
              value="74"
              />
          <task
              name="ESE_CacheMemoryUsage_Trace"
              value="75"
              />
          <task
              name="ESE_CacheSetLgposModify_Trace"
              value="76"
              />
          <!--ESE_ETW_AUTOGEN_TASK_LIST_END-->
          <!--ESE_ETW_TRACETAG_NAMEVALUES: Inserted here. All tracetag tasks values start at 100, see genetw.pl.-->
        </tasks>

        <!-- ==========================================================================================================
                      MAPS
                    (in instrumentation\events\provider\maps)
             ==========================================================================================================
          -->
        <maps>
            <!--ESE_ETW_JETAPI_NAMEMAP: Inserted here.  See genetw.pl-->
            <valueMap name="ESE_DirtyLevelMap">
              <map value="0"    message="$(string.DirtyLevelMap.bfdfClean)"/>
              <map value="1"    message="$(string.DirtyLevelMap.bfdfUntidy)"/>
              <map value="2"    message="$(string.DirtyLevelMap.bfdfDirty)"/>
              <map value="3"    message="$(string.DirtyLevelMap.bfdfFilthy)"/>
            </valueMap>
            <!--ESE_ETW_IORP_NAMEMAP: Inserted here.  See genetw.pl-->
            <!--ESE_ETW_IORT_NAMEMAP: Inserted here.  See genetw.pl-->
            <!--ESE_ETW_IORS_NAMEMAP: Inserted here.  See genetw.pl-->
            <!--ESE_ETW_IORF_NAMEMAP: Inserted here.  See genetw.pl-->
            <bitMap name="ESE_PageFlagsMap">
              <map value="0x0000"  message="$(string.PageFlagsMap.fPagePrimary)"/>
              <map value="0x0001"  message="$(string.PageFlagsMap.fPageRoot)"/>
              <map value="0x0002"  message="$(string.PageFlagsMap.fPageLeaf)"/>
              <map value="0x0004"  message="$(string.PageFlagsMap.fPageParentOfLeaf)"/>
              <map value="0x0008"  message="$(string.PageFlagsMap.fPageEmpty)"/>
              <map value="0x0010"  message="$(string.PageFlagsMap.fPageRepair)"/>
              <map value="0x0020"  message="$(string.PageFlagsMap.fPageSpaceTree)"/>
              <map value="0x0040"  message="$(string.PageFlagsMap.fPageIndex)"/>
              <map value="0x0080"  message="$(string.PageFlagsMap.fPageLongValue)"/>
              <map value="0x0400"  message="$(string.PageFlagsMap.fPageNonUniqueKeys)"/>
              <map value="0x0800"  message="$(string.PageFlagsMap.fPageNewRecordFormat)"/>
              <map value="0x2000"  message="$(string.PageFlagsMap.fPageNewChecksumFormat)"/>
              <map value="0x4000"  message="$(string.PageFlagsMap.fPageScrubbed)"/>
              <map value="0x8000"  message="$(string.PageFlagsMap.flushTypeBit1)"/>
              <map value="0x10000" message="$(string.PageFlagsMap.flushTypeBit2)"/>
              <map value="0x20000" message="$(string.PageFlagsMap.fPagePreInit)"/>
            </bitMap>
            <!--ESE_ETW_IOFR_NAMEMAP: Inserted here.  See genetw.pl-->
            <!--ESE_ETW_IOFILE_NAMEMAP: Inserted here.  See genetw.pl-->
        </maps>

        <!-- ==========================================================================================================
                      TEMPLATES
                    (in instrumentation\events\provider\templates)
             ==========================================================================================================
              Templates may be shared across multiple events, if they share the same data payload.
              For valid inType / outType combinations, please see the Remarks section in 
              http://msdn.microsoft.com/en-us/library/aa382774(VS.85).aspx
              For more complex data types, please use a tool like EcManGen.exe from the Win7 SDK
              to author manifests
          -->
        <templates>

          <!-- This template ... I/SOMEONE can't find a reference, means it is uselss right? -->
          <template tid="tStringTrace">
            <data  inType="win:AnsiString"          name="szTrace"            />
          </template>

          <!--ESE_ETW_AUTOGEN_TEMPLATE_LIST_BEGIN-->
          <!-- DO NOT EDIT = THESE TEMPLATES AUTO-GEN (from EseEtwEventsPregen.txt) -->
          <template tid="tCacheNewPageTrace">
            <data  inType="win:UInt32"              name="ifmp"               />
            <data  inType="win:UInt32"              name="pgno"               />
            <data  inType="win:UInt32"              name="LatchFlags"         outType="win:HexInt32"             />
            <data  inType="win:UInt32"              name="objid"              />
            <data  inType="win:UInt32"              name="PageFlags"          outType="win:HexInt32"             map="ESE_PageFlagsMap"  />
            <data  inType="win:UInt32"              name="UserId"             outType="win:HexInt32"             />
            <data  inType="win:UInt8"               name="OperationId"        />
            <data  inType="win:UInt8"               name="OperationType"      />
            <data  inType="win:UInt8"               name="ClientType"         />
            <data  inType="win:UInt8"               name="Flags"              outType="win:HexInt8"              />
            <data  inType="win:UInt32"              name="CorrelationId"      outType="win:HexInt32"             />
            <data  inType="win:UInt8"               name="Iorp"               map="ESE_IorpMap"                  />
            <data  inType="win:UInt8"               name="Iors"               map="ESE_IorsMap"                  />
            <data  inType="win:UInt8"               name="Iort"               map="ESE_IortMap"                  />
            <data  inType="win:UInt8"               name="Ioru"               />
            <data  inType="win:UInt8"               name="Iorf"               outType="win:HexInt8"              map="ESE_IorfMap"  />
            <data  inType="win:UInt8"               name="ParentObjectClass"  />
            <data  inType="win:UInt64"              name="dbtimeDirtied"      />
            <data  inType="win:UInt16"              name="itagMicFree"        />
            <data  inType="win:UInt16"              name="cbFree"             />
          </template>
          <template tid="tCacheReadPageTrace">
            <data  inType="win:UInt32"              name="ifmp"               />
            <data  inType="win:UInt32"              name="pgno"               />
            <data  inType="win:UInt32"              name="LatchFlags"         outType="win:HexInt32"             />
            <data  inType="win:UInt32"              name="objid"              />
            <data  inType="win:UInt32"              name="PageFlags"          outType="win:HexInt32"             map="ESE_PageFlagsMap"  />
            <data  inType="win:UInt32"              name="UserId"             outType="win:HexInt32"             />
            <data  inType="win:UInt8"               name="OperationId"        />
            <data  inType="win:UInt8"               name="OperationType"      />
            <data  inType="win:UInt8"               name="ClientType"         />
            <data  inType="win:UInt8"               name="Flags"              outType="win:HexInt8"              />
            <data  inType="win:UInt32"              name="CorrelationId"      outType="win:HexInt32"             />
            <data  inType="win:UInt8"               name="Iorp"               map="ESE_IorpMap"                  />
            <data  inType="win:UInt8"               name="Iors"               map="ESE_IorsMap"                  />
            <data  inType="win:UInt8"               name="Iort"               map="ESE_IortMap"                  />
            <data  inType="win:UInt8"               name="Ioru"               />
            <data  inType="win:UInt8"               name="Iorf"               outType="win:HexInt8"              map="ESE_IorfMap"  />
            <data  inType="win:UInt8"               name="ParentObjectClass"  />
            <data  inType="win:UInt64"              name="dbtimeDirtied"      />
            <data  inType="win:UInt16"              name="itagMicFree"        />
            <data  inType="win:UInt16"              name="cbFree"             />
          </template>
          <template tid="tCachePrereadPageTrace">
            <data  inType="win:UInt32"              name="ifmp"               />
            <data  inType="win:UInt32"              name="pgno"               />
            <data  inType="win:UInt32"              name="UserId"             outType="win:HexInt32"             />
            <data  inType="win:UInt8"               name="OperationId"        />
            <data  inType="win:UInt8"               name="OperationType"      />
            <data  inType="win:UInt8"               name="ClientType"         />
            <data  inType="win:UInt8"               name="Flags"              outType="win:HexInt8"              />
            <data  inType="win:UInt32"              name="CorrelationId"      outType="win:HexInt32"             />
            <data  inType="win:UInt8"               name="Iorp"               map="ESE_IorpMap"                  />
            <data  inType="win:UInt8"               name="Iors"               map="ESE_IorsMap"                  />
            <data  inType="win:UInt8"               name="Iort"               map="ESE_IortMap"                  />
            <data  inType="win:UInt8"               name="Ioru"               />
            <data  inType="win:UInt8"               name="Iorf"               outType="win:HexInt8"              map="ESE_IorfMap"  />
            <data  inType="win:UInt8"               name="ParentObjectClass"  />
          </template>
          <template tid="tCacheWritePageTrace">
            <data  inType="win:UInt32"              name="tick"               />
            <data  inType="win:UInt32"              name="ifmp"               />
            <data  inType="win:UInt32"              name="pgno"               />
            <data  inType="win:UInt32"              name="objid"              />
            <data  inType="win:UInt32"              name="PageFlags"          outType="win:HexInt32"             map="ESE_PageFlagsMap"  />
            <data  inType="win:UInt32"              name="DirtyLevel"         map="ESE_DirtyLevelMap"            />
            <data  inType="win:UInt32"              name="UserId"             outType="win:HexInt32"             />
            <data  inType="win:UInt8"               name="OperationId"        />
            <data  inType="win:UInt8"               name="OperationType"      />
            <data  inType="win:UInt8"               name="ClientType"         />
            <data  inType="win:UInt8"               name="Flags"              outType="win:HexInt8"              />
            <data  inType="win:UInt32"              name="CorrelationId"      outType="win:HexInt32"             />
            <data  inType="win:UInt8"               name="Iorp"               map="ESE_IorpMap"                  />
            <data  inType="win:UInt8"               name="Iors"               map="ESE_IorsMap"                  />
            <data  inType="win:UInt8"               name="Iort"               map="ESE_IortMap"                  />
            <data  inType="win:UInt8"               name="Ioru"               />
            <data  inType="win:UInt8"               name="Iorf"               outType="win:HexInt8"              map="ESE_IorfMap"  />
            <data  inType="win:UInt8"               name="ParentObjectClass"  />
          </template>
          <template tid="tCacheEvictPageTrace">
            <data  inType="win:UInt32"              name="tick"               />
            <data  inType="win:UInt32"              name="ifmp"               />
            <data  inType="win:UInt32"              name="pgno"               />
            <data  inType="win:UInt32"              name="fCurrentVersion"    />
            <data  inType="win:Int32"               name="errBF"              />
            <data  inType="win:UInt32"              name="bfef"               outType="win:HexInt32"             />
            <data  inType="win:UInt32"              name="pctPriority"        />
          </template>
          <template tid="tCacheRequestPageTrace">
            <data  inType="win:UInt32"              name="tick"               />
            <data  inType="win:UInt32"              name="ifmp"               />
            <data  inType="win:UInt32"              name="pgno"               />
            <data  inType="win:UInt32"              name="bflf"               outType="win:HexInt32"             />
            <data  inType="win:UInt32"              name="objid"              />
            <data  inType="win:UInt32"              name="PageFlags"          outType="win:HexInt32"             map="ESE_PageFlagsMap"  />
            <data  inType="win:UInt32"              name="bflt"               />
            <data  inType="win:UInt32"              name="pctPriority"        />
            <data  inType="win:UInt32"              name="bfrtf"              />
            <data  inType="win:UInt8"               name="ClientType"         />
          </template>
          <template tid="tCacheDirtyPageTrace">
            <data  inType="win:UInt32"              name="tick"               />
            <data  inType="win:UInt32"              name="ifmp"               />
            <data  inType="win:UInt32"              name="pgno"               />
            <data  inType="win:UInt32"              name="objid"              />
            <data  inType="win:UInt32"              name="PageFlags"          outType="win:HexInt32"             map="ESE_PageFlagsMap"  />
            <data  inType="win:UInt32"              name="DirtyLevel"         map="ESE_DirtyLevelMap"            />
            <data  inType="win:UInt64"              name="LgposModify"        outType="win:HexInt64"             />
            <data  inType="win:UInt32"              name="UserId"             outType="win:HexInt32"             />
            <data  inType="win:UInt8"               name="OperationId"        />
            <data  inType="win:UInt8"               name="OperationType"      />
            <data  inType="win:UInt8"               name="ClientType"         />
            <data  inType="win:UInt8"               name="Flags"              outType="win:HexInt8"              />
            <data  inType="win:UInt32"              name="CorrelationId"      outType="win:HexInt32"             />
            <data  inType="win:UInt8"               name="Iorp"               map="ESE_IorpMap"                  />
            <data  inType="win:UInt8"               name="Iors"               map="ESE_IorsMap"                  />
            <data  inType="win:UInt8"               name="Iort"               map="ESE_IortMap"                  />
            <data  inType="win:UInt8"               name="Ioru"               />
            <data  inType="win:UInt8"               name="Iorf"               outType="win:HexInt8"              map="ESE_IorfMap"  />
            <data  inType="win:UInt8"               name="ParentObjectClass"  />
          </template>
          <template tid="tTransactionBeginTrace">
            <data  inType="win:Pointer"             name="SessionNumber"      />
            <data  inType="win:Pointer"             name="TransactionNumber"  />
            <data  inType="win:UInt8"               name="TransactionLevel"   />
          </template>
          <template tid="tTransactionCommitTrace">
            <data  inType="win:Pointer"             name="SessionNumber"      />
            <data  inType="win:Pointer"             name="TransactionNumber"  />
            <data  inType="win:UInt8"               name="TransactionLevel"   />
          </template>
          <template tid="tTransactionRollbackTrace">
            <data  inType="win:Pointer"             name="SessionNumber"      />
            <data  inType="win:Pointer"             name="TransactionNumber"  />
            <data  inType="win:UInt8"               name="TransactionLevel"   />
          </template>
          <template tid="tSpaceAllocExtTrace">
            <data  inType="win:UInt32"              name="ifmp"               />
            <data  inType="win:UInt32"              name="pgnoFDP"            />
            <data  inType="win:UInt32"              name="pgnoFirst"          />
            <data  inType="win:UInt32"              name="cpg"                />
            <data  inType="win:UInt32"              name="objidFDP"           />
            <data  inType="win:UInt8"               name="tce"                />
          </template>
          <template tid="tSpaceFreeExtTrace">
            <data  inType="win:UInt32"              name="ifmp"               />
            <data  inType="win:UInt32"              name="pgnoFDP"            />
            <data  inType="win:UInt32"              name="pgnoFirst"          />
            <data  inType="win:UInt32"              name="cpg"                />
            <data  inType="win:UInt32"              name="objidFDP"           />
            <data  inType="win:UInt8"               name="tce"                />
          </template>
          <template tid="tSpaceAllocPageTrace">
            <data  inType="win:UInt32"              name="ifmp"               />
            <data  inType="win:UInt32"              name="pgnoFDP"            />
            <data  inType="win:UInt32"              name="pgnoAlloc"          />
            <data  inType="win:UInt32"              name="objidFDP"           />
            <data  inType="win:UInt8"               name="tce"                />
          </template>
          <template tid="tSpaceFreePageTrace">
            <data  inType="win:UInt32"              name="ifmp"               />
            <data  inType="win:UInt32"              name="pgnoFDP"            />
            <data  inType="win:UInt32"              name="pgnoFree"           />
            <data  inType="win:UInt32"              name="objidFDP"           />
            <data  inType="win:UInt8"               name="tce"                />
          </template>
          <template tid="tIorunEnqueueTrace">
            <data  inType="win:UInt64"              name="iFile"              />
            <data  inType="win:UInt64"              name="ibOffset"           />
            <data  inType="win:UInt32"              name="cbData"             />
            <data  inType="win:UInt32"              name="tidAlloc"           />
            <data  inType="win:UInt32"              name="fHeapA"             />
            <data  inType="win:UInt32"              name="fWrite"             />
            <data  inType="win:UInt32"              name="EngineFileType"     map="ESE_IofileMap"                />
            <data  inType="win:UInt64"              name="EngineFileId"       />
            <data  inType="win:UInt32"              name="cusecEnqueueLatency" />
          </template>
          <template tid="tIorunDequeueTrace">
            <data  inType="win:UInt64"              name="iFile"              />
            <data  inType="win:UInt64"              name="ibOffset"           />
            <data  inType="win:UInt32"              name="cbData"             />
            <data  inType="win:UInt32"              name="tidAlloc"           />
            <data  inType="win:UInt32"              name="fHeapA"             />
            <data  inType="win:UInt32"              name="fWrite"             />
            <data  inType="win:UInt32"              name="Iorp"               map="ESE_IorpMap"                  />
            <data  inType="win:UInt32"              name="Iors"               map="ESE_IorsMap"                  />
            <data  inType="win:UInt32"              name="Ioru"               />
            <data  inType="win:UInt32"              name="Iorf"               outType="win:HexInt32"             map="ESE_IorfMap"                />
            <data  inType="win:UInt32"              name="grbitQos"           outType="win:HexInt32"             />
            <data  inType="win:UInt64"              name="cmsecTimeInQueue"   />
            <data  inType="win:UInt32"              name="EngineFileType"     map="ESE_IofileMap"                />
            <data  inType="win:UInt64"              name="EngineFileId"       />
            <data  inType="win:UInt64"              name="cDispatchPass"      />
            <data  inType="win:UInt16"              name="cIorunCombined"     />
            <data  inType="win:UInt32"              name="cusecDequeueLatency"  />
          </template>
          <template tid="tIOCompletionTrace">
            <data  inType="win:UInt64"              name="iFile"              />
            <data  inType="win:UInt32"              name="fMultiIor"          />
            <data  inType="win:UInt8"               name="fWrite"             />
            <data  inType="win:UInt32"              name="UserId"             outType="win:HexInt32"             />
            <data  inType="win:UInt8"               name="OperationId"        />
            <data  inType="win:UInt8"               name="OperationType"      />
            <data  inType="win:UInt8"               name="ClientType"         />
            <data  inType="win:UInt8"               name="Flags"              outType="win:HexInt8"              />
            <data  inType="win:UInt32"              name="CorrelationId"      outType="win:HexInt32"             />
            <data  inType="win:UInt8"               name="Iorp"               map="ESE_IorpMap"                  />
            <data  inType="win:UInt8"               name="Iors"               map="ESE_IorsMap"                  />
            <data  inType="win:UInt8"               name="Iort"               map="ESE_IortMap"                  />
            <data  inType="win:UInt8"               name="Ioru"               />
            <data  inType="win:UInt8"               name="Iorf"               outType="win:HexInt8"              map="ESE_IorfMap"                />
            <data  inType="win:UInt8"               name="ParentObjectClass"  />
            <data  inType="win:UInt64"              name="ibOffset"           />
            <data  inType="win:UInt32"              name="cbTransfer"         />
            <data  inType="win:UInt32"              name="error"              />
            <data  inType="win:UInt32"              name="qosHighestFirst"    outType="win:HexInt32"             />
            <data  inType="win:UInt64"              name="cmsecIOElapsed"     />
            <data  inType="win:UInt32"              name="dtickQueueDelay"    />
            <data  inType="win:UInt32"              name="tidAlloc"           />
            <data  inType="win:UInt32"              name="EngineFileType"     map="ESE_IofileMap"                />
            <data  inType="win:UInt64"              name="EngineFileId"       />
            <data  inType="win:UInt32"              name="fmfFile"            />
            <data  inType="win:UInt32"              name="DiskNumber"         />
            <data  inType="win:UInt32"              name="dwEngineObjid"      />
            <data  inType="win:UInt64"              name="qosIOComplete"      />
          </template>
          <template tid="tLogWriteTrace">
            <data  inType="win:Int32"               name="lgenData"           />
            <data  inType="win:UInt32"              name="ibLogData"          />
            <data  inType="win:UInt32"              name="cbLogData"          />
          </template>
          <template tid="tEventLogInfoTrace">
            <data  inType="win:UnicodeString"       name="szTrace"            />
          </template>
          <template tid="tEventLogWarnTrace">
            <data  inType="win:UnicodeString"       name="szTrace"            />
          </template>
          <template tid="tEventLogErrorTrace">
            <data  inType="win:UnicodeString"       name="szTrace"            />
          </template>
          <template tid="tTimerTaskScheduleTrace">
            <data  inType="win:Pointer"             name="posttTimerHandle"   />
            <data  inType="win:Pointer"             name="pfnTask"            />
            <data  inType="win:Pointer"             name="pvTaskGroupContext" />
            <data  inType="win:Pointer"             name="pvRuntimeContext"   />
            <data  inType="win:UInt32"              name="dtickMinDelay"      />
            <data  inType="win:UInt32"              name="dtickSlopDelay"     />
          </template>
          <template tid="tTimerTaskRunTrace">
            <data  inType="win:Pointer"             name="posttTimerHandle"   />
            <data  inType="win:Pointer"             name="pfnTask"            />
            <data  inType="win:Pointer"             name="pvTaskGroupContext" />
            <data  inType="win:Pointer"             name="pvRuntimeContext"   />
            <data  inType="win:UInt64"              name="cRuns"              />
          </template>
          <template tid="tTimerTaskCancelTrace">
            <data  inType="win:Pointer"             name="posttTimerHandle"   />
            <data  inType="win:Pointer"             name="pfnTask"            />
            <data  inType="win:Pointer"             name="pvTaskGroupContext" />
          </template>
          <template tid="tTaskManagerPostTrace">
            <data  inType="win:Pointer"             name="ptm"                />
            <data  inType="win:Pointer"             name="pfnCompletion"      />
            <data  inType="win:UInt32"              name="dwCompletionKey1"   />
            <data  inType="win:Pointer"             name="dwCompletionKey2"   />
          </template>
          <template tid="tTaskManagerRunTrace">
            <data  inType="win:Pointer"             name="ptm"                />
            <data  inType="win:Pointer"             name="pfnCompletion"      />
            <data  inType="win:UInt32"              name="dwCompletionKey1"   />
            <data  inType="win:Pointer"             name="dwCompletionKey2"   />
            <data  inType="win:UInt32"              name="gle"                />
            <data  inType="win:Pointer"             name="dwThreadContext"    />
          </template>
          <template tid="tGPTaskManagerPostTrace">
            <data  inType="win:Pointer"             name="pgptm"              />
            <data  inType="win:Pointer"             name="pfnCompletion"      />
            <data  inType="win:Pointer"             name="pvParam"            />
            <data  inType="win:Pointer"             name="pTaskInfo"          />
          </template>
          <template tid="tGPTaskManagerRunTrace">
            <data  inType="win:Pointer"             name="pgptm"              />
            <data  inType="win:Pointer"             name="pfnCompletion"      />
            <data  inType="win:Pointer"             name="pvParam"            />
            <data  inType="win:Pointer"             name="pTaskInfo"          />
          </template>
          <template tid="tTestMarkerTrace">
            <data  inType="win:UInt64"              name="qwMarkerID"         />
            <data  inType="win:AnsiString"          name="szAnnotation"       />
          </template>
          <template tid="tThreadCreateTrace">
            <data  inType="win:Pointer"             name="Thread"             />
            <data  inType="win:Pointer"             name="pfnStart"           />
            <data  inType="win:Pointer"             name="dwParam"            />
          </template>
          <template tid="tThreadStartTrace">
            <data  inType="win:Pointer"             name="Thread"             />
            <data  inType="win:Pointer"             name="pfnStart"           />
            <data  inType="win:Pointer"             name="dwParam"            />
          </template>
          <template tid="tCacheVersionPageTrace">
            <data  inType="win:UInt32"              name="ifmp"               />
            <data  inType="win:UInt32"              name="pgno"               />
          </template>
          <template tid="tCacheVersionCopyPageTrace">
            <data  inType="win:UInt32"              name="ifmp"               />
            <data  inType="win:UInt32"              name="pgno"               />
          </template>
          <template tid="tCacheResizeTrace">
            <data  inType="win:Int64"               name="cbfCacheAddressableInitial" />
            <data  inType="win:Int64"               name="cbfCacheSizeInitial" />
            <data  inType="win:Int64"               name="cbfCacheAddressableFinal" />
            <data  inType="win:Int64"               name="cbfCacheSizeFinal"  />
          </template>
          <template tid="tCacheLimitResizeTrace">
            <data  inType="win:Int64"               name="cbfCacheSizeLimitInitial" />
            <data  inType="win:Int64"               name="cbfCacheSizeLimitFinal" />
          </template>
          <template tid="tCacheScavengeProgressTrace">
            <data  inType="win:Int64"               name="iRun"               />
            <data  inType="win:Int32"               name="cbfVisited"         />
            <data  inType="win:Int32"               name="cbfCacheSize"       />
            <data  inType="win:Int32"               name="cbfCacheTarget"     />
            <data  inType="win:Int32"               name="cbfCacheSizeStartShrink" />
            <data  inType="win:UInt32"              name="dtickShrinkDuration" />
            <data  inType="win:Int32"               name="cbfAvail"           />
            <data  inType="win:Int32"               name="cbfAvailPoolLow"    />
            <data  inType="win:Int32"               name="cbfAvailPoolHigh"   />
            <data  inType="win:Int32"               name="cbfFlushPending"    />
            <data  inType="win:Int32"               name="cbfFlushPendingSlow" />
            <data  inType="win:Int32"               name="cbfFlushPendingHung" />
            <data  inType="win:Int32"               name="cbfOutOfMemory"     />
            <data  inType="win:Int32"               name="cbfPermanentErrs"   />
            <data  inType="win:Int32"               name="eStopReason"        />
            <data  inType="win:Int32"               name="errRun"             />
          </template>
          <template tid="tApiCall_StartTrace">
            <data  inType="win:UInt32"              name="opApi"              map="ESE_ApiCallNameMap"           />
          </template>
          <template tid="tApiCall_StopTrace">
            <data  inType="win:UInt32"              name="opApi"              map="ESE_ApiCallNameMap"           />
            <data  inType="win:Int32"               name="err"                />
          </template>
          <template tid="tResMgrInitTrace">
            <data  inType="win:UInt32"              name="tick"               />
            <data  inType="win:Int32"               name="K"                  />
            <data  inType="win:Double"              name="csecCorrelatedTouch" />
            <data  inType="win:Double"              name="csecTimeout"        />
            <data  inType="win:Double"              name="csecUncertainty"    />
            <data  inType="win:Double"              name="dblHashLoadFactor"  />
            <data  inType="win:Double"              name="dblHashUniformity"  />
            <data  inType="win:Double"              name="dblSpeedSizeTradeoff" />
          </template>
          <template tid="tResMgrTermTrace">
            <data  inType="win:UInt32"              name="tick"               />
          </template>
          <template tid="tCacheCachePageTrace">
            <data  inType="win:UInt32"              name="tick"               />
            <data  inType="win:UInt32"              name="ifmp"               />
            <data  inType="win:UInt32"              name="pgno"               />
            <data  inType="win:UInt32"              name="bflf"               outType="win:HexInt32"             />
            <data  inType="win:UInt32"              name="bflt"               />
            <data  inType="win:UInt32"              name="pctPriority"        />
            <data  inType="win:UInt32"              name="bfrtf"              />
            <data  inType="win:UInt8"               name="ClientType"         />
          </template>
          <template tid="tMarkPageAsSuperColdTrace">
            <data  inType="win:UInt32"              name="tick"               />
            <data  inType="win:UInt32"              name="ifmp"               />
            <data  inType="win:UInt32"              name="pgno"               />
          </template>
          <template tid="tCacheMissLatencyTrace">
            <data  inType="win:UInt32"              name="ifmp"               />
            <data  inType="win:UInt32"              name="pgno"               />
            <data  inType="win:UInt32"              name="dwUserId"           outType="win:HexInt32"             />
            <data  inType="win:UInt8"               name="bOperationId"       />
            <data  inType="win:UInt8"               name="bOperationType"     />
            <data  inType="win:UInt8"               name="bClientType"        />
            <data  inType="win:UInt8"               name="bFlags"             outType="win:HexInt8"              />
            <data  inType="win:UInt32"              name="dwCorrelationId"    outType="win:HexInt32"             />
            <data  inType="win:UInt8"               name="iorp"               map="ESE_IorpMap"                  />
            <data  inType="win:UInt8"               name="iors"               map="ESE_IorsMap"                  />
            <data  inType="win:UInt8"               name="iort"               map="ESE_IortMap"                  />
            <data  inType="win:UInt8"               name="ioru"               />
            <data  inType="win:UInt8"               name="iorf"               outType="win:HexInt8"              map="ESE_IorfMap"  />
            <data  inType="win:UInt8"               name="tce"                />
            <data  inType="win:UInt64"              name="usecsWait"          />
            <data  inType="win:UInt8"               name="bftcmr"             />
            <data  inType="win:UInt8"               name="bUserPriorityTag"   />
          </template>
          <template tid="tBTreePrereadPageRequestTrace">
            <data  inType="win:UInt32"              name="ifmp"               />
            <data  inType="win:UInt32"              name="pgno"               />
            <data  inType="win:UInt32"              name="dwUserId"           outType="win:HexInt32"             />
            <data  inType="win:UInt8"               name="bOperationId"       />
            <data  inType="win:UInt8"               name="bOperationType"     />
            <data  inType="win:UInt8"               name="bClientType"        />
            <data  inType="win:UInt8"               name="bFlags"             outType="win:HexInt8"              />
            <data  inType="win:UInt32"              name="dwCorrelationId"    outType="win:HexInt32"             />
            <data  inType="win:UInt8"               name="iorp"               map="ESE_IorpMap"                  />
            <data  inType="win:UInt8"               name="iors"               map="ESE_IorsMap"                  />
            <data  inType="win:UInt8"               name="iort"               map="ESE_IortMap"                  />
            <data  inType="win:UInt8"               name="ioru"               />
            <data  inType="win:UInt8"               name="iorf"               outType="win:HexInt8"              map="ESE_IorfMap"  />
            <data  inType="win:UInt8"               name="tce"                />
            <data  inType="win:UInt8"               name="fOpFlags"           />
          </template>
          <template tid="tDiskFlushFileBuffersTrace">
            <data  inType="win:UInt32"              name="Disk"               />
            <data  inType="win:UnicodeString"       name="wszFileName"        />
            <data  inType="win:UInt32"              name="iofr"               map="ESE_IofrMap"                  />
            <data  inType="win:UInt64"              name="cioreqFileFlushing" />
            <data  inType="win:UInt64"              name="usFfb"              />
            <data  inType="win:UInt32"              name="error"              />
          </template>
          <template tid="tDiskFlushFileBuffersBeginTrace">
            <data  inType="win:UInt32"              name="dwDisk"             />
            <data  inType="win:UInt64"              name="hFile"              />
            <data  inType="win:UInt32"              name="iofr"               map="ESE_IofrMap"                  />
          </template>
          <template tid="tCacheFirstDirtyPageTrace">
            <data  inType="win:UInt32"              name="tick"               />
            <data  inType="win:UInt32"              name="ifmp"               />
            <data  inType="win:UInt32"              name="pgno"               />
            <data  inType="win:UInt32"              name="objid"              />
            <data  inType="win:UInt32"              name="fFlags"             outType="win:HexInt32"             map="ESE_PageFlagsMap"  />
            <data  inType="win:UInt32"              name="bfdfNew"            map="ESE_DirtyLevelMap"            />
            <data  inType="win:UInt64"              name="lgposModify"        outType="win:HexInt64"             />
            <data  inType="win:UInt32"              name="dwUserId"           outType="win:HexInt32"             />
            <data  inType="win:UInt8"               name="bOperationId"       />
            <data  inType="win:UInt8"               name="bOperationType"     />
            <data  inType="win:UInt8"               name="bClientType"        />
            <data  inType="win:UInt8"               name="bFlags"             outType="win:HexInt8"              />
            <data  inType="win:UInt32"              name="dwCorrelationId"    outType="win:HexInt32"             />
            <data  inType="win:UInt8"               name="iorp"               map="ESE_IorpMap"                  />
            <data  inType="win:UInt8"               name="iors"               map="ESE_IorsMap"                  />
            <data  inType="win:UInt8"               name="iort"               map="ESE_IortMap"                  />
            <data  inType="win:UInt8"               name="ioru"               />
            <data  inType="win:UInt8"               name="iorf"               outType="win:HexInt8"              map="ESE_IorfMap"  />
            <data  inType="win:UInt8"               name="tce"                />
          </template>
          <template tid="tSysStationIdTrace">
            <data  inType="win:UInt8"               name="tsidr"              />
            <data  inType="win:UInt32"              name="dwImageVerMajor"    />
            <data  inType="win:UInt32"              name="dwImageVerMinor"    />
            <data  inType="win:UInt32"              name="dwImageBuildMajor"  />
            <data  inType="win:UInt32"              name="dwImageBuildMinor"  />
            <data  inType="win:UnicodeString"       name="wszDisplayName"     />
          </template>
          <template tid="tInstStationIdTrace">
            <data  inType="win:UInt8"               name="tsidr"              />
            <data  inType="win:UInt32"              name="iInstance"          />
            <data  inType="win:UInt8"               name="perfstatusEvent"    />
            <data  inType="win:UnicodeString"       name="wszInstanceName"    />
            <data  inType="win:UnicodeString"       name="wszDisplayName"     />
          </template>
          <template tid="tFmpStationIdTrace">
            <data  inType="win:UInt8"               name="tsidr"              />
            <data  inType="win:UInt32"              name="ifmp"               />
            <data  inType="win:UInt32"              name="iInstance"          />
            <data  inType="win:UInt8"               name="dbid"               />
            <data  inType="win:UnicodeString"       name="wszDatabaseName"    />
          </template>
          <template tid="tDiskStationIdTrace">
            <data  inType="win:UInt8"               name="tsidr"              />
            <data  inType="win:UInt32"              name="dwDiskNumber"       />
            <data  inType="win:UnicodeString"       name="wszDiskPathId"      />
            <data  inType="win:AnsiString"          name="szDiskModel"        />
            <data  inType="win:AnsiString"          name="szDiskFirmwareRev"  />
            <data  inType="win:AnsiString"          name="szDiskSerialNumber" />
          </template>
          <template tid="tFileStationIdTrace">
            <data  inType="win:UInt8"               name="tsidr"              />
            <data  inType="win:UInt64"              name="hFile"              />
            <data  inType="win:UInt32"              name="dwDiskNumber"       />
            <data  inType="win:UInt32"              name="dwEngineFileType"   map="ESE_IofileMap"                />
            <data  inType="win:UInt64"              name="qwEngineFileId"     />
            <data  inType="win:UInt32"              name="fmf"                />
            <data  inType="win:UInt64"              name="cbFileSize"         />
            <data  inType="win:UnicodeString"       name="wszAbsPath"         />
          </template>
          <template tid="tIsamDbfilehdrInfoTrace">
            <data  inType="win:UInt8"               name="tsidr"              />
            <data  inType="win:UInt32"              name="ifmp"               />
            <data  inType="win:UInt32"              name="filetype"           />
            <data  inType="win:UInt32"              name="ulMagic"            />
            <data  inType="win:UInt32"              name="ulChecksum"         />
            <data  inType="win:UInt32"              name="cbPageSize"         />
            <data  inType="win:UInt32"              name="ulDbFlags"          />
            <data  inType="win:Binary" length="28"  name="psignDb"            />
          </template>
          <template tid="tDiskOsDiskCacheInfoTrace">
            <data  inType="win:UInt8"               name="tsidr"              />
            <data  inType="win:Binary" length="24"  name="posdci"             />
          </template>
          <template tid="tDiskOsStorageWriteCachePropTrace">
            <data  inType="win:UInt8"               name="tsidr"              />
            <data  inType="win:Binary" length="28"  name="posswcp"            />
          </template>
          <template tid="tDiskOsDeviceSeekPenaltyDescTrace">
            <data  inType="win:UInt8"               name="tsidr"              />
            <data  inType="win:Binary" length="12"  name="posdspd"            />
          </template>
          <template tid="tIOCompletion2Trace">
            <data  inType="win:UnicodeString"       name="wszFilename"        />
            <data  inType="win:UInt32"              name="fMultiIor"          />
            <data  inType="win:UInt8"               name="fWrite"             />
            <data  inType="win:UInt32"              name="dwUserId"           outType="win:HexInt32"             />
            <data  inType="win:UInt8"               name="bOperationId"       />
            <data  inType="win:UInt8"               name="bOperationType"     />
            <data  inType="win:UInt8"               name="bClientType"        />
            <data  inType="win:UInt8"               name="bFlags"             outType="win:HexInt8"              />
            <data  inType="win:UInt32"              name="dwCorrelationId"    outType="win:HexInt32"             />
            <data  inType="win:UInt8"               name="iorp"               map="ESE_IorpMap"                  />
            <data  inType="win:UInt8"               name="iors"               map="ESE_IorsMap"                  />
            <data  inType="win:UInt8"               name="iort"               map="ESE_IortMap"                  />
            <data  inType="win:UInt8"               name="ioru"               />
            <data  inType="win:UInt8"               name="iorf"               outType="win:HexInt8"              map="ESE_IorfMap"                  />
            <data  inType="win:UInt8"               name="tce"                />
            <data  inType="win:AnsiString"          name="szClientComponent"  />
            <data  inType="win:AnsiString"          name="szClientAction"     />
            <data  inType="win:AnsiString"          name="szClientActionContext"/>
            <data  inType="win:GUID"                name="guidActivityId"     />
            <data  inType="win:UInt64"              name="ibOffset"           />
            <data  inType="win:UInt32"              name="cbTransfer"         />
            <data  inType="win:UInt32"              name="dwError"            />
            <data  inType="win:UInt32"              name="qosHighestFirst"    outType="win:HexInt32"             />
            <data  inType="win:UInt64"              name="cmsecIOElapsed"     />
            <data  inType="win:UInt32"              name="dtickQueueDelay"    />
            <data  inType="win:UInt32"              name="tidAlloc"           />
            <data  inType="win:UInt32"              name="dwEngineFileType"   map="ESE_IofileMap"                />
            <data  inType="win:UInt64"              name="dwEngineFileId"     />
            <data  inType="win:UInt32"              name="fmfFile"            />
            <data  inType="win:UInt32"              name="dwDiskNumber"       />
            <data  inType="win:UInt32"              name="dwEngineObjid"      />
          </template>
          <template tid="tFCBPurgeFailureTrace">
            <data  inType="win:UInt32"              name="iInstance"          />
            <data  inType="win:UInt8"               name="grbitPurgeFlags"    />
            <data  inType="win:UInt8"               name="fcbpfr"             />
            <data  inType="win:UInt8"               name="tce"                />
          </template>
          <template tid="tIOLatencySpikeNoticeTrace">
            <data  inType="win:UInt32"              name="dwDiskNumber"       />
            <data  inType="win:UInt32"              name="dtickSpikeLength"   />
          </template>
          <template tid="tIOCompletion2SessTrace">
            <data  inType="win:UnicodeString"       name="wszFilename"        />
            <data  inType="win:UInt32"              name="fMultiIor"          />
            <data  inType="win:UInt8"               name="fWrite"             />
            <data  inType="win:UInt32"              name="dwUserId"           outType="win:HexInt32"             />
            <data  inType="win:UInt8"               name="bOperationId"       />
            <data  inType="win:UInt8"               name="bOperationType"     />
            <data  inType="win:UInt8"               name="bClientType"        />
            <data  inType="win:UInt8"               name="bFlags"             outType="win:HexInt8"              />
            <data  inType="win:UInt32"              name="dwCorrelationId"    outType="win:HexInt32"             />
            <data  inType="win:UInt8"               name="iorp"               map="ESE_IorpMap"                  />
            <data  inType="win:UInt8"               name="iors"               map="ESE_IorsMap"                  />
            <data  inType="win:UInt8"               name="iort"               map="ESE_IortMap"                  />
            <data  inType="win:UInt8"               name="ioru"               />
            <data  inType="win:UInt8"               name="iorf"               outType="win:HexInt8"              map="ESE_IorfMap"                  />
            <data  inType="win:UInt8"               name="tce"                />
            <data  inType="win:AnsiString"          name="szClientComponent"  />
            <data  inType="win:AnsiString"          name="szClientAction"     />
            <data  inType="win:AnsiString"          name="szClientActionContext"/>
            <data  inType="win:GUID"                name="guidActivityId"     />
            <data  inType="win:UInt64"              name="ibOffset"           />
            <data  inType="win:UInt32"              name="cbTransfer"         />
            <data  inType="win:UInt32"              name="dwError"            />
            <data  inType="win:UInt32"              name="qosHighestFirst"    outType="win:HexInt32"             />
            <data  inType="win:UInt64"              name="cmsecIOElapsed"     />
            <data  inType="win:UInt32"              name="dtickQueueDelay"    />
            <data  inType="win:UInt32"              name="tidAlloc"           />
            <data  inType="win:UInt32"              name="dwEngineFileType"   map="ESE_IofileMap"                />
            <data  inType="win:UInt64"              name="dwEngineFileId"     />
            <data  inType="win:UInt32"              name="fmfFile"            />
            <data  inType="win:UInt32"              name="dwDiskNumber"       />
            <data  inType="win:UInt32"              name="dwEngineObjid"      />
          </template>
          <template tid="tIOIssueThreadPostTrace">
            <data  inType="win:Pointer"             name="p_osf"              />
            <data  inType="win:UInt32"              name="cioDiskEnqueued"    />
          </template>
          <template tid="tIOIssueThreadPostedTrace">
            <data  inType="win:Pointer"             name="p_osf"              />
            <data  inType="win:UInt32"              name="cDispatchAttempts"  />
            <data  inType="win:UInt64"              name="usPosted"           />
          </template>
          <template tid="tIOThreadIssuedDiskTrace">
            <data  inType="win:UInt32"              name="dwDiskId"           />
            <data  inType="win:UInt8"               name="fFromCompletion"    />
            <data  inType="win:Int64"               name="ipass"              />
            <data  inType="win:Int32"               name="err"                />
            <data  inType="win:UInt32"              name="cioProcessed"       />
            <data  inType="win:UInt64"              name="usRuntime"          />
          </template>
          <template tid="tIOThreadIssueProcessedIOTrace">
            <data  inType="win:Int32"               name="err"                />
            <data  inType="win:UInt32"              name="cDisksProcessed"    />
            <data  inType="win:UInt64"              name="usRuntime"          />
          </template>
          <template tid="tIOIoreqCompletionTrace">
            <data  inType="win:UInt8"               name="fWrite"             />
            <data  inType="win:UInt64"              name="iFile"              />
            <data  inType="win:UInt64"              name="ibOffset"           />
            <data  inType="win:UInt32"              name="cbData"             />
            <data  inType="win:UInt32"              name="dwDiskNumber"       />
            <data  inType="win:UInt32"              name="tidAlloc"           />
            <data  inType="win:UInt64"              name="qos"                outType="win:HexInt64"             />
            <data  inType="win:UInt32"              name="iIoreq"             />
            <data  inType="win:UInt32"              name="err"                />
            <data  inType="win:UInt64"              name="usCompletionDelay"  />
          </template>
          <template tid="tCacheMemoryUsageTrace">
            <data  inType="win:UnicodeString"       name="wszFilename"        />
            <data  inType="win:UInt8"               name="tce"                />
            <data  inType="win:UInt32"              name="dwEngineFileType"   map="ESE_IofileMap"                />
            <data  inType="win:UInt64"              name="dwEngineFileId"     />
            <data  inType="win:UInt64"              name="cbMemory"           />
            <data  inType="win:UInt32"              name="cmsecReferenceIntervalMax" />
          </template>
          <template tid="tCacheSetLgposModifyTrace">
            <data  inType="win:UInt32"              name="tick"               />
            <data  inType="win:UInt32"              name="ifmp"               />
            <data  inType="win:UInt32"              name="pgno"               />
            <data  inType="win:UInt64"              name="lgposModify"        outType="win:HexInt64"             />
          </template>
          <!--ESE_ETW_AUTOGEN_TEMPLATE_LIST_END-->
          <template tid="tESE_CompressionExperiment_Trace">
            <data
                inType="win:UInt8"
                name="TableClass"
                />
            <data
                inType="win:UInt16"
                name="OrigSize"
                />
            <data
                inType="win:UInt16"
                name="XpressSize"
                />
            <data
                inType="win:UInt16"
                name="Xpress9Size"
                />
            <data
                inType="win:UInt32"
                name="usecXpressTime"
                />
            <data
                inType="win:UInt32"
                name="usecXpress9Time"
                />
            <data
                inType="win:Double"
                name="ShannonEntropy"
                />
            <data
                inType="win:Double"
                name="ChiSquared"
                />
          </template>
        </templates>

        <!-- ==========================================================================================================
                      EVENTS
                    (in instrumentation\events\provider\events)
             ==========================================================================================================
              The actual events, though much of the data is in the templates above.
          -->
        <events>
          <!--ESE_ETW_AUTOGEN_EVENT_LIST_BEGIN-->
            <!-- DO NOT EDIT = THESE EVENTS AUTO-GEN (from EseEtwEventsPregen.txt) -->
          <event
              keywords="Trace"
              level="win:Verbose"
              message="$(string.Event.ESE_TraceBaseId_Trace)"
              symbol="ESE_TraceBaseId_Trace"
              task="ESE_TraceBaseId_Trace"
              value="100"
              />
          <event
              keywords="Trace"
              level="win:Verbose"
              message="$(string.Event.ESE_BF_Trace)"
              symbol="ESE_BF_Trace"
              task="ESE_BF_Trace"
              value="101"
              />
          <event
              keywords="BF"
              level="win:Verbose"
              message="$(string.Event.ESE_Block_Trace)"
              symbol="ESE_Block_Trace"
              task="ESE_Block_Trace"
              value="102"
              />
          <event
              keywords="BF Performance DataWorkingSet"
              level="win:Informational"
              message="$(string.Event.ESE_CacheNewPage_Trace)"
              symbol="ESE_CacheNewPage_Trace"
              task="ESE_CacheNewPage_Trace"
              template="tCacheNewPageTrace"
              value="103"
              />
          <event
              keywords="BF Performance DataWorkingSet"
              level="win:Informational"
              message="$(string.Event.ESE_CacheReadPage_Trace)"
              symbol="ESE_CacheReadPage_Trace"
              task="ESE_CacheReadPage_Trace"
              template="tCacheReadPageTrace"
              value="104"
              />
          <event
              keywords="BF Performance DataWorkingSet"
              level="win:Informational"
              message="$(string.Event.ESE_CachePrereadPage_Trace)"
              symbol="ESE_CachePrereadPage_Trace"
              task="ESE_CachePrereadPage_Trace"
              template="tCachePrereadPageTrace"
              value="105"
              />
          <event
              keywords="BF BFRESMGR Performance DataWorkingSet"
              level="win:Informational"
              message="$(string.Event.ESE_CacheWritePage_Trace)"
              symbol="ESE_CacheWritePage_Trace"
              task="ESE_CacheWritePage_Trace"
              template="tCacheWritePageTrace"
              value="106"
              />
          <event
              keywords="BF BFRESMGR DataWorkingSet"
              level="win:Informational"
              message="$(string.Event.ESE_CacheEvictPage_Trace)"
              symbol="ESE_CacheEvictPage_Trace"
              task="ESE_CacheEvictPage_Trace"
              template="tCacheEvictPageTrace"
              value="107"
              />
          <event
              keywords="BF BFRESMGR"
              level="win:Verbose"
              message="$(string.Event.ESE_CacheRequestPage_Trace)"
              symbol="ESE_CacheRequestPage_Trace"
              task="ESE_CacheRequestPage_Trace"
              template="tCacheRequestPageTrace"
              value="108"
              />
          <event
              keywords="BF BFRESMGR Performance"
              level="win:Informational"
              message="$(string.Event.ESE_LatchPageDeprecated_Trace)"
              symbol="ESE_LatchPageDeprecated_Trace"
              task="ESE_LatchPageDeprecated_Trace"
              value="109"
              />
          <event
              keywords="BF BFRESMGR Performance"
              level="win:Verbose"
              message="$(string.Event.ESE_CacheDirtyPage_Trace)"
              symbol="ESE_CacheDirtyPage_Trace"
              task="ESE_CacheDirtyPage_Trace"
              template="tCacheDirtyPageTrace"
              value="110"
              />
          <event
              keywords="Transaction"
              level="win:Verbose"
              message="$(string.Event.ESE_TransactionBegin_Trace)"
              symbol="ESE_TransactionBegin_Trace"
              task="ESE_TransactionBegin_Trace"
              template="tTransactionBeginTrace"
              value="111"
              />
          <event
              keywords="Transaction"
              level="win:Verbose"
              message="$(string.Event.ESE_TransactionCommit_Trace)"
              symbol="ESE_TransactionCommit_Trace"
              task="ESE_TransactionCommit_Trace"
              template="tTransactionCommitTrace"
              value="112"
              />
          <event
              keywords="Transaction"
              level="win:Verbose"
              message="$(string.Event.ESE_TransactionRollback_Trace)"
              symbol="ESE_TransactionRollback_Trace"
              task="ESE_TransactionRollback_Trace"
              template="tTransactionRollbackTrace"
              value="113"
              />
          <event
              keywords="Space"
              level="win:Verbose"
              message="$(string.Event.ESE_SpaceAllocExt_Trace)"
              symbol="ESE_SpaceAllocExt_Trace"
              task="ESE_SpaceAllocExt_Trace"
              template="tSpaceAllocExtTrace"
              value="114"
              />
          <event
              keywords="Space"
              level="win:Verbose"
              message="$(string.Event.ESE_SpaceFreeExt_Trace)"
              symbol="ESE_SpaceFreeExt_Trace"
              task="ESE_SpaceFreeExt_Trace"
              template="tSpaceFreeExtTrace"
              value="115"
              />
          <event
              keywords="Space DataWorkingSet"
              level="win:Verbose"
              message="$(string.Event.ESE_SpaceAllocPage_Trace)"
              symbol="ESE_SpaceAllocPage_Trace"
              task="ESE_SpaceAllocPage_Trace"
              template="tSpaceAllocPageTrace"
              value="116"
              />
          <event
              keywords="Space DataWorkingSet"
              level="win:Verbose"
              message="$(string.Event.ESE_SpaceFreePage_Trace)"
              symbol="ESE_SpaceFreePage_Trace"
              task="ESE_SpaceFreePage_Trace"
              template="tSpaceFreePageTrace"
              value="117"
              />
          <event
              keywords="IO"
              level="win:Informational"
              message="$(string.Event.ESE_IorunEnqueue_Trace)"
              symbol="ESE_IorunEnqueue_Trace"
              task="ESE_IorunEnqueue_Trace"
              template="tIorunEnqueueTrace"
              value="118"
              channel="IODiagnoseChannel"
              />
          <event
              keywords="IO"
              level="win:Informational"
              message="$(string.Event.ESE_IorunDequeue_Trace)"
              symbol="ESE_IorunDequeue_Trace"
              task="ESE_IorunDequeue_Trace"
              template="tIorunDequeueTrace"
              value="119"
              channel="IODiagnoseChannel"
              />
          <event
              keywords="IO"
              level="win:Informational"
              message="$(string.Event.ESE_IOCompletion_Trace)"
              symbol="ESE_IOCompletion_Trace"
              task="ESE_IOCompletion_Trace"
              template="tIOCompletionTrace"
              value="120"
              channel="IODiagnoseChannel"
              />
          <event
              keywords="LOG StallLatencies"
              level="win:Warning"
              message="$(string.Event.ESE_LogStall_Trace)"
              symbol="ESE_LogStall_Trace"
              task="ESE_LogStall_Trace"
              value="121"
              />
          <event
              keywords="LOG"
              level="win:Informational"
              message="$(string.Event.ESE_LogWrite_Trace)"
              symbol="ESE_LogWrite_Trace"
              task="ESE_LogWrite_Trace"
              template="tLogWriteTrace"
              value="122"
              />
          <event
              keywords="Trace"
              level="win:Informational"
              message="$(string.Event.ESE_EventLogInfo_Trace)"
              symbol="ESE_EventLogInfo_Trace"
              task="ESE_EventLogInfo_Trace"
              template="tEventLogInfoTrace"
              value="123"
              channel="OpChannel"
              />
          <event
              keywords="Trace"
              level="win:Warning"
              message="$(string.Event.ESE_EventLogWarn_Trace)"
              symbol="ESE_EventLogWarn_Trace"
              task="ESE_EventLogWarn_Trace"
              template="tEventLogWarnTrace"
              value="124"
              channel="OpChannel"
              />
          <event
              keywords="Trace"
              level="win:Error"
              message="$(string.Event.ESE_EventLogError_Trace)"
              symbol="ESE_EventLogError_Trace"
              task="ESE_EventLogError_Trace"
              template="tEventLogErrorTrace"
              value="125"
              channel="OpChannel"
              />
          <event
              keywords="Task"
              level="win:Verbose"
              message="$(string.Event.ESE_TimerQueueScheduleDeprecated_Trace)"
              symbol="ESE_TimerQueueScheduleDeprecated_Trace"
              task="ESE_TimerQueueScheduleDeprecated_Trace"
              value="126"
              />
          <event
              keywords="Task"
              level="win:Verbose"
              message="$(string.Event.ESE_TimerQueueRunDeprecated_Trace)"
              symbol="ESE_TimerQueueRunDeprecated_Trace"
              task="ESE_TimerQueueRunDeprecated_Trace"
              value="127"
              />
          <event
              keywords="Task"
              level="win:Verbose"
              message="$(string.Event.ESE_TimerQueueCancelDeprecated_Trace)"
              symbol="ESE_TimerQueueCancelDeprecated_Trace"
              task="ESE_TimerQueueCancelDeprecated_Trace"
              value="128"
              />
          <event
              keywords="Task"
              level="win:Verbose"
              message="$(string.Event.ESE_TimerTaskSchedule_Trace)"
              symbol="ESE_TimerTaskSchedule_Trace"
              task="ESE_TimerTaskSchedule_Trace"
              template="tTimerTaskScheduleTrace"
              value="129"
              />
          <event
              keywords="Task"
              level="win:Verbose"
              message="$(string.Event.ESE_TimerTaskRun_Trace)"
              symbol="ESE_TimerTaskRun_Trace"
              task="ESE_TimerTaskRun_Trace"
              template="tTimerTaskRunTrace"
              value="130"
              />
          <event
              keywords="Task"
              level="win:Verbose"
              message="$(string.Event.ESE_TimerTaskCancel_Trace)"
              symbol="ESE_TimerTaskCancel_Trace"
              task="ESE_TimerTaskCancel_Trace"
              template="tTimerTaskCancelTrace"
              value="131"
              />
          <event
              keywords="Task"
              level="win:Verbose"
              message="$(string.Event.ESE_TaskManagerPost_Trace)"
              symbol="ESE_TaskManagerPost_Trace"
              task="ESE_TaskManagerPost_Trace"
              template="tTaskManagerPostTrace"
              value="132"
              />
          <event
              keywords="Task"
              level="win:Verbose"
              message="$(string.Event.ESE_TaskManagerRun_Trace)"
              symbol="ESE_TaskManagerRun_Trace"
              task="ESE_TaskManagerRun_Trace"
              template="tTaskManagerRunTrace"
              value="133"
              />
          <event
              keywords="Task"
              level="win:Verbose"
              message="$(string.Event.ESE_GPTaskManagerPost_Trace)"
              symbol="ESE_GPTaskManagerPost_Trace"
              task="ESE_GPTaskManagerPost_Trace"
              template="tGPTaskManagerPostTrace"
              value="134"
              />
          <event
              keywords="Task"
              level="win:Verbose"
              message="$(string.Event.ESE_GPTaskManagerRun_Trace)"
              symbol="ESE_GPTaskManagerRun_Trace"
              task="ESE_GPTaskManagerRun_Trace"
              template="tGPTaskManagerRunTrace"
              value="135"
              />
          <event
              keywords="Test"
              level="win:Informational"
              message="$(string.Event.ESE_TestMarker_Trace)"
              symbol="ESE_TestMarker_Trace"
              task="ESE_TestMarker_Trace"
              template="tTestMarkerTrace"
              value="136"
              />
          <event
              keywords="Task"
              level="win:Informational"
              message="$(string.Event.ESE_ThreadCreate_Trace)"
              symbol="ESE_ThreadCreate_Trace"
              task="ESE_ThreadCreate_Trace"
              template="tThreadCreateTrace"
              value="137"
              />
          <event
              keywords="Task"
              level="win:Informational"
              message="$(string.Event.ESE_ThreadStart_Trace)"
              symbol="ESE_ThreadStart_Trace"
              task="ESE_ThreadStart_Trace"
              template="tThreadStartTrace"
              value="138"
              />
          <event
              keywords="BF Performance"
              level="win:Informational"
              message="$(string.Event.ESE_CacheVersionPage_Trace)"
              symbol="ESE_CacheVersionPage_Trace"
              task="ESE_CacheVersionPage_Trace"
              template="tCacheVersionPageTrace"
              value="139"
              />
          <event
              keywords="BF Performance"
              level="win:Informational"
              message="$(string.Event.ESE_CacheVersionCopyPage_Trace)"
              symbol="ESE_CacheVersionCopyPage_Trace"
              task="ESE_CacheVersionCopyPage_Trace"
              template="tCacheVersionCopyPageTrace"
              value="140"
              />
          <event
              keywords="BF Performance"
              level="win:Informational"
              message="$(string.Event.ESE_CacheResize_Trace)"
              symbol="ESE_CacheResize_Trace"
              task="ESE_CacheResize_Trace"
              template="tCacheResizeTrace"
              value="141"
              />
          <event
              keywords="BF Performance"
              level="win:Informational"
              message="$(string.Event.ESE_CacheLimitResize_Trace)"
              symbol="ESE_CacheLimitResize_Trace"
              task="ESE_CacheLimitResize_Trace"
              template="tCacheLimitResizeTrace"
              value="142"
              />
          <event
              keywords="BF Performance"
              level="win:Informational"
              message="$(string.Event.ESE_CacheScavengeProgress_Trace)"
              symbol="ESE_CacheScavengeProgress_Trace"
              task="ESE_CacheScavengeProgress_Trace"
              template="tCacheScavengeProgressTrace"
              value="143"
              />
          <event
              keywords="Performance"
              level="win:Informational"
              message="$(string.Event.ESE_ApiCall_Start_Trace)"
              symbol="ESE_ApiCall_Start_Trace"
              task="ESE_ApiCall_Trace"
              opcode="win:Start"
              template="tApiCall_StartTrace"
              value="144"
              />
          <event
              keywords="Performance"
              level="win:Informational"
              message="$(string.Event.ESE_ApiCall_Stop_Trace)"
              symbol="ESE_ApiCall_Stop_Trace"
              task="ESE_ApiCall_Trace"
              opcode="win:Stop"
              template="tApiCall_StopTrace"
              value="145"
              />
          <event
              keywords="BFRESMGR"
              level="win:Informational"
              message="$(string.Event.ESE_ResMgrInit_Trace)"
              symbol="ESE_ResMgrInit_Trace"
              task="ESE_ResMgrInit_Trace"
              template="tResMgrInitTrace"
              value="146"
              />
          <event
              keywords="BFRESMGR"
              level="win:Informational"
              message="$(string.Event.ESE_ResMgrTerm_Trace)"
              symbol="ESE_ResMgrTerm_Trace"
              task="ESE_ResMgrTerm_Trace"
              template="tResMgrTermTrace"
              value="147"
              />
          <event
              keywords="BF BFRESMGR"
              level="win:Verbose"
              message="$(string.Event.ESE_CacheCachePage_Trace)"
              symbol="ESE_CacheCachePage_Trace"
              task="ESE_CacheCachePage_Trace"
              template="tCacheCachePageTrace"
              value="148"
              />
          <event
              keywords="BF BFRESMGR"
              level="win:Verbose"
              message="$(string.Event.ESE_MarkPageAsSuperCold_Trace)"
              symbol="ESE_MarkPageAsSuperCold_Trace"
              task="ESE_MarkPageAsSuperCold_Trace"
              template="tMarkPageAsSuperColdTrace"
              value="149"
              />
          <event
              keywords="BF StallLatencies"
              level="win:Informational"
              message="$(string.Event.ESE_CacheMissLatency_Trace)"
              symbol="ESE_CacheMissLatency_Trace"
              task="ESE_CacheMissLatency_Trace"
              template="tCacheMissLatencyTrace"
              value="150"
              />
          <event
              keywords="Performance"
              level="win:Informational"
              message="$(string.Event.ESE_BTreePrereadPageRequest_Trace)"
              symbol="ESE_BTreePrereadPageRequest_Trace"
              task="ESE_BTreePrereadPageRequest_Trace"
              template="tBTreePrereadPageRequestTrace"
              value="151"
              />
          <event
              keywords="IO"
              level="win:Informational"
              message="$(string.Event.ESE_DiskFlushFileBuffers_Trace)"
              symbol="ESE_DiskFlushFileBuffers_Trace"
              task="ESE_DiskFlushFileBuffers_Trace"
              template="tDiskFlushFileBuffersTrace"
              value="152"
              />
          <event
              keywords="IO"
              level="win:Informational"
              message="$(string.Event.ESE_DiskFlushFileBuffersBegin_Trace)"
              symbol="ESE_DiskFlushFileBuffersBegin_Trace"
              task="ESE_DiskFlushFileBuffersBegin_Trace"
              template="tDiskFlushFileBuffersBeginTrace"
              value="153"
              />
          <event
              keywords="BF DataWorkingSet"
              level="win:Informational"
              message="$(string.Event.ESE_CacheFirstDirtyPage_Trace)"
              symbol="ESE_CacheFirstDirtyPage_Trace"
              task="ESE_CacheFirstDirtyPage_Trace"
              template="tCacheFirstDirtyPageTrace"
              value="154"
              />
          <event
              keywords="StationId"
              level="win:Informational"
              message="$(string.Event.ESE_SysStationId_Trace)"
              symbol="ESE_SysStationId_Trace"
              task="ESE_SysStationId_Trace"
              template="tSysStationIdTrace"
              value="155"
              />
          <event
              keywords="StationId"
              level="win:Informational"
              message="$(string.Event.ESE_InstStationId_Trace)"
              symbol="ESE_InstStationId_Trace"
              task="ESE_InstStationId_Trace"
              template="tInstStationIdTrace"
              value="156"
              />
          <event
              keywords="StationId"
              level="win:Informational"
              message="$(string.Event.ESE_FmpStationId_Trace)"
              symbol="ESE_FmpStationId_Trace"
              task="ESE_FmpStationId_Trace"
              template="tFmpStationIdTrace"
              value="157"
              />
          <event
              keywords="StationId"
              level="win:Informational"
              message="$(string.Event.ESE_DiskStationId_Trace)"
              symbol="ESE_DiskStationId_Trace"
              task="ESE_DiskStationId_Trace"
              template="tDiskStationIdTrace"
              value="158"
              />
          <event
              keywords="StationId"
              level="win:Informational"
              message="$(string.Event.ESE_FileStationId_Trace)"
              symbol="ESE_FileStationId_Trace"
              task="ESE_FileStationId_Trace"
              template="tFileStationIdTrace"
              value="159"
              />
          <event
              keywords="StationId"
              level="win:Informational"
              message="$(string.Event.ESE_IsamDbfilehdrInfo_Trace)"
              symbol="ESE_IsamDbfilehdrInfo_Trace"
              task="ESE_IsamDbfilehdrInfo_Trace"
              template="tIsamDbfilehdrInfoTrace"
              value="160"
              />
          <event
              keywords="StationId"
              level="win:Informational"
              message="$(string.Event.ESE_DiskOsDiskCacheInfo_Trace)"
              symbol="ESE_DiskOsDiskCacheInfo_Trace"
              task="ESE_DiskOsDiskCacheInfo_Trace"
              template="tDiskOsDiskCacheInfoTrace"
              value="161"
              />
          <event
              keywords="StationId"
              level="win:Informational"
              message="$(string.Event.ESE_DiskOsStorageWriteCacheProp_Trace)"
              symbol="ESE_DiskOsStorageWriteCacheProp_Trace"
              task="ESE_DiskOsStorageWriteCacheProp_Trace"
              template="tDiskOsStorageWriteCachePropTrace"
              value="162"
              />
          <event
              keywords="StationId"
              level="win:Informational"
              message="$(string.Event.ESE_DiskOsDeviceSeekPenaltyDesc_Trace)"
              symbol="ESE_DiskOsDeviceSeekPenaltyDesc_Trace"
              task="ESE_DiskOsDeviceSeekPenaltyDesc_Trace"
              template="tDiskOsDeviceSeekPenaltyDescTrace"
              value="163"
              />
          <event
              keywords=""
              level="win:Informational"
              message="$(string.Event.ESE_DirtyPage2Deprecated_Trace)"
              symbol="ESE_DirtyPage2Deprecated_Trace"
              task="ESE_DirtyPage2Deprecated_Trace"
              value="164"
              />
          <event
              keywords="IOEX"
              level="win:Informational"
              message="$(string.Event.ESE_IOCompletion2_Trace)"
              symbol="ESE_IOCompletion2_Trace"
              task="ESE_IOCompletion2_Trace"
              template="tIOCompletion2Trace"
              value="165"
              />
          <event
              keywords=""
              level="win:Informational"
              message="$(string.Event.ESE_FCBPurgeFailure_Trace)"
              symbol="ESE_FCBPurgeFailure_Trace"
              task="ESE_FCBPurgeFailure_Trace"
              template="tFCBPurgeFailureTrace"
              value="166"
              />
          <event
              keywords="StallLatencies"
              level="win:Informational"
              message="$(string.Event.ESE_IOLatencySpikeNotice_Trace)"
              symbol="ESE_IOLatencySpikeNotice_Trace"
              task="ESE_IOLatencySpikeNotice_Trace"
              template="tIOLatencySpikeNoticeTrace"
              value="167"
              />
          <event
              keywords="IOSESS"
              level="win:Informational"
              message="$(string.Event.ESE_IOCompletion2Sess_Trace)"
              symbol="ESE_IOCompletion2Sess_Trace"
              task="ESE_IOCompletion2Sess_Trace"
              template="tIOCompletion2SessTrace"
              value="168"
              />
          <event
              keywords="IO"
              level="win:Informational"
              message="$(string.Event.ESE_IOIssueThreadPost_Trace)"
              symbol="ESE_IOIssueThreadPost_Trace"
              task="ESE_IOIssueThreadPost_Trace"
              template="tIOIssueThreadPostTrace"
              value="169"
              />
          <event
              keywords="IO"
              level="win:Informational"
              message="$(string.Event.ESE_IOIssueThreadPosted_Trace)"
              symbol="ESE_IOIssueThreadPosted_Trace"
              task="ESE_IOIssueThreadPosted_Trace"
              template="tIOIssueThreadPostedTrace"
              value="170"
              />
          <event
              keywords="IO"
              level="win:Informational"
              message="$(string.Event.ESE_IOThreadIssueStart_Trace)"
              symbol="ESE_IOThreadIssueStart_Trace"
              task="ESE_IOThreadIssueStart_Trace"
              value="171"
              />
          <event
              keywords="IO"
              level="win:Informational"
              message="$(string.Event.ESE_IOThreadIssuedDisk_Trace)"
              symbol="ESE_IOThreadIssuedDisk_Trace"
              task="ESE_IOThreadIssuedDisk_Trace"
              template="tIOThreadIssuedDiskTrace"
              value="172"
              />
          <event
              keywords="IO"
              level="win:Informational"
              message="$(string.Event.ESE_IOThreadIssueProcessedIO_Trace)"
              symbol="ESE_IOThreadIssueProcessedIO_Trace"
              task="ESE_IOThreadIssueProcessedIO_Trace"
              template="tIOThreadIssueProcessedIOTrace"
              value="173"
              />
          <event
              keywords="IO StallLatencies"
              level="win:Informational"
              message="$(string.Event.ESE_IOIoreqCompletion_Trace)"
              symbol="ESE_IOIoreqCompletion_Trace"
              task="ESE_IOIoreqCompletion_Trace"
              template="tIOIoreqCompletionTrace"
              value="174"
              />
          <event
              keywords="SubstrateTelemetry"
              level="win:Informational"
              message="$(string.Event.ESE_CacheMemoryUsage_Trace)"
              symbol="ESE_CacheMemoryUsage_Trace"
              task="ESE_CacheMemoryUsage_Trace"
              template="tCacheMemoryUsageTrace"
              value="175"
              />
          <event
              keywords="BF BFRESMGR"
              level="win:Verbose"
              message="$(string.Event.ESE_CacheSetLgposModify_Trace)"
              symbol="ESE_CacheSetLgposModify_Trace"
              task="ESE_CacheSetLgposModify_Trace"
              template="tCacheSetLgposModifyTrace"
              value="176"
              />
          <!--ESE_ETW_AUTOGEN_EVENT_LIST_END-->

          <!-- Temporary trace for collecting data for a compression experiment -->
          <event
              keywords="CompressExp"
              level="win:Verbose"
              message="$(string.Event.ESE_CompressionExperiment_Trace)"
              symbol="ESE_CompressionExperiment_Trace"
              template="tESE_CompressionExperiment_Trace"
              value="5000"
              />

          <!--ESE_ETW_TRACETAG_EVENTS: Inserted here. All events values start at 200, see genetw.pl. -->
        </events>
      </provider>
    </events>
  </instrumentation>
  <memberships>
    <categoryMembership>
      <id
          name="Microsoft.Windows.Categories"
          publicKeyToken="365143bb27e7ac8b"
          typeName="BootRecovery"
          version="1.0.0.0"
          />
    </categoryMembership>
  </memberships>
  <localization>
    <resources culture="en-US">
      <!-- ==========================================================================================================
                    STRING TABLE
                  (in instrumentation\localization\resources\stringTable)
           ==========================================================================================================
            Miscellaneous strings to IDs we can convert. 
        -->
      <stringTable>
        <string
            id="Keyword.Error"
            value="Error"
            />
        <string
            id="Keyword.Performance"
            value="Performance"
            />
        <!-- Define my own keywords here -->
        <string
            id="Keyword.Trace"
            value="Trace"
            />
        <string
            id="Keyword.Transaction"
            value="Transaction"
            />
        <string
            id="Keyword.Space"
            value="Space"
            />
        <string
            id="Keyword.BF"
            value="BF"
            />
        <string
            id="Keyword.IO"
            value="IO"
            />
        <string
            id="Keyword.LOG"
            value="LOG"
            />
        <string
            id="Keyword.Task"
            value="Task"
            />
        <string
            id="Keyword.Test"
            value="Test"
            />
        <string
            id="Keyword.BFRESMGR"
            value="BFRESMGR"
            />
        <string
            id="Keyword.StationId"
            value="StationId"
            />
        <string
            id="Keyword.JETTraceTag"
            value="JETTraceTag"
            />
        <string
            id="Keyword.StallLatencies"
            value="StallLatencies"
            />
        <string
            id="Keyword.DataWorkingSet"
            value="DataWorkbccingSet"
            />
        <string
            id="Keyword.IOEX"
            value="IOEX"
            />
        <string
            id="Keyword.IOSESS"
            value="IOSESS"
            />
        <string
            id="Keyword.SubstrateTelemetry"
            value="SubstrateTelemetry"
            />
        <string
            id="Keyword.CompressExp"
            value="CompressExp"
            />
        <!--ESE_ETW_AUTOGEN_STRING_LIST_BEGIN-->
            <!-- DO NOT EDIT = THESE STRINGS AUTO-GEN (from EseEtwEventsPregen.txt) -->
        <string
            id="Event.ESE_TraceBaseId_Trace"
            value="ESE TraceBaseId Trace"
            />
        <string
            id="Event.ESE_BF_Trace"
            value="ESE BF Trace"
            />
        <string
            id="Event.ESE_Block_Trace"
            value="ESE Block Trace"
            />
        <string
            id="Event.ESE_CacheNewPage_Trace"
            value="ESE CacheNewPage Trace"
            />
        <string
            id="Event.ESE_CacheReadPage_Trace"
            value="ESE CacheReadPage Trace"
            />
        <string
            id="Event.ESE_CachePrereadPage_Trace"
            value="ESE CachePrereadPage Trace"
            />
        <string
            id="Event.ESE_CacheWritePage_Trace"
            value="ESE CacheWritePage Trace"
            />
        <string
            id="Event.ESE_CacheEvictPage_Trace"
            value="ESE CacheEvictPage Trace"
            />
        <string
            id="Event.ESE_CacheRequestPage_Trace"
            value="ESE CacheRequestPage Trace"
            />
        <string
            id="Event.ESE_LatchPageDeprecated_Trace"
            value="ESE LatchPageDeprecated Trace"
            />
        <string
            id="Event.ESE_CacheDirtyPage_Trace"
            value="ESE CacheDirtyPage Trace"
            />
        <string
            id="Event.ESE_TransactionBegin_Trace"
            value="ESE TransactionBegin Trace"
            />
        <string
            id="Event.ESE_TransactionCommit_Trace"
            value="ESE TransactionCommit Trace"
            />
        <string
            id="Event.ESE_TransactionRollback_Trace"
            value="ESE TransactionRollback Trace"
            />
        <string
            id="Event.ESE_SpaceAllocExt_Trace"
            value="ESE SpaceAllocExt Trace"
            />
        <string
            id="Event.ESE_SpaceFreeExt_Trace"
            value="ESE SpaceFreeExt Trace"
            />
        <string
            id="Event.ESE_SpaceAllocPage_Trace"
            value="ESE SpaceAllocPage Trace"
            />
        <string
            id="Event.ESE_SpaceFreePage_Trace"
            value="ESE SpaceFreePage Trace"
            />
        <string
            id="Event.ESE_IorunEnqueue_Trace"
            value="ESE IorunEnqueue Trace"
            />
        <string
            id="Event.ESE_IorunDequeue_Trace"
            value="ESE IorunDequeue Trace"
            />
        <string
            id="Event.ESE_IOCompletion_Trace"
            value="ESE IOCompletion Trace"
            />
        <string
            id="Event.ESE_LogStall_Trace"
            value="ESE LogStall Trace"
            />
        <string
            id="Event.ESE_LogWrite_Trace"
            value="ESE LogWrite Trace"
            />
        <string
            id="Event.ESE_EventLogInfo_Trace"
            value="ESE EventLogInfo Trace"
            />
        <string
            id="Event.ESE_EventLogWarn_Trace"
            value="ESE EventLogWarn Trace"
            />
        <string
            id="Event.ESE_EventLogError_Trace"
            value="ESE EventLogError Trace"
            />
        <string
            id="Event.ESE_TimerQueueScheduleDeprecated_Trace"
            value="ESE TimerQueueScheduleDeprecated Trace"
            />
        <string
            id="Event.ESE_TimerQueueRunDeprecated_Trace"
            value="ESE TimerQueueRunDeprecated Trace"
            />
        <string
            id="Event.ESE_TimerQueueCancelDeprecated_Trace"
            value="ESE TimerQueueCancelDeprecated Trace"
            />
        <string
            id="Event.ESE_TimerTaskSchedule_Trace"
            value="ESE TimerTaskSchedule Trace"
            />
        <string
            id="Event.ESE_TimerTaskRun_Trace"
            value="ESE TimerTaskRun Trace"
            />
        <string
            id="Event.ESE_TimerTaskCancel_Trace"
            value="ESE TimerTaskCancel Trace"
            />
        <string
            id="Event.ESE_TaskManagerPost_Trace"
            value="ESE TaskManagerPost Trace"
            />
        <string
            id="Event.ESE_TaskManagerRun_Trace"
            value="ESE TaskManagerRun Trace"
            />
        <string
            id="Event.ESE_GPTaskManagerPost_Trace"
            value="ESE GPTaskManagerPost Trace"
            />
        <string
            id="Event.ESE_GPTaskManagerRun_Trace"
            value="ESE GPTaskManagerRun Trace"
            />
        <string
            id="Event.ESE_TestMarker_Trace"
            value="ESE TestMarker Trace"
            />
        <string
            id="Event.ESE_ThreadCreate_Trace"
            value="ESE ThreadCreate Trace"
            />
        <string
            id="Event.ESE_ThreadStart_Trace"
            value="ESE ThreadStart Trace"
            />
        <string
            id="Event.ESE_CacheVersionPage_Trace"
            value="ESE CacheVersionPage Trace"
            />
        <string
            id="Event.ESE_CacheVersionCopyPage_Trace"
            value="ESE CacheVersionCopyPage Trace"
            />
        <string
            id="Event.ESE_CacheResize_Trace"
            value="ESE CacheResize Trace"
            />
        <string
            id="Event.ESE_CacheLimitResize_Trace"
            value="ESE CacheLimitResize Trace"
            />
        <string
            id="Event.ESE_CacheScavengeProgress_Trace"
            value="ESE CacheScavengeProgress Trace"
            />
        <string
            id="Event.ESE_ApiCall_Start_Trace"
            value="ESE ApiCall_Start Trace"
            />
        <string
            id="Event.ESE_ApiCall_Stop_Trace"
            value="ESE ApiCall_Stop Trace"
            />
        <string
            id="Event.ESE_ResMgrInit_Trace"
            value="ESE ResMgrInit Trace"
            />
        <string
            id="Event.ESE_ResMgrTerm_Trace"
            value="ESE ResMgrTerm Trace"
            />
        <string
            id="Event.ESE_CacheCachePage_Trace"
            value="ESE CacheCachePage Trace"
            />
        <string
            id="Event.ESE_MarkPageAsSuperCold_Trace"
            value="ESE MarkPageAsSuperCold Trace"
            />
        <string
            id="Event.ESE_CacheMissLatency_Trace"
            value="ESE CacheMissLatency Trace"
            />
        <string
            id="Event.ESE_BTreePrereadPageRequest_Trace"
            value="ESE BTreePrereadPageRequest Trace"
            />
        <string
            id="Event.ESE_DiskFlushFileBuffers_Trace"
            value="ESE DiskFlushFileBuffers Trace"
            />
        <string
            id="Event.ESE_DiskFlushFileBuffersBegin_Trace"
            value="ESE DiskFlushFileBuffersBegin Trace"
            />
        <string
            id="Event.ESE_CacheFirstDirtyPage_Trace"
            value="ESE CacheFirstDirtyPage Trace"
            />
        <string
            id="Event.ESE_SysStationId_Trace"
            value="ESE SysStationId Trace"
            />
        <string
            id="Event.ESE_InstStationId_Trace"
            value="ESE InstStationId Trace"
            />
        <string
            id="Event.ESE_FmpStationId_Trace"
            value="ESE FmpStationId Trace"
            />
        <string
            id="Event.ESE_DiskStationId_Trace"
            value="ESE DiskStationId Trace"
            />
        <string
            id="Event.ESE_FileStationId_Trace"
            value="ESE FileStationId Trace"
            />
        <string
            id="Event.ESE_IsamDbfilehdrInfo_Trace"
            value="ESE IsamDbfilehdrInfo Trace"
            />
        <string
            id="Event.ESE_DiskOsDiskCacheInfo_Trace"
            value="ESE DiskOsDiskCacheInfo Trace"
            />
        <string
            id="Event.ESE_DiskOsStorageWriteCacheProp_Trace"
            value="ESE DiskOsStorageWriteCacheProp Trace"
            />
        <string
            id="Event.ESE_DiskOsDeviceSeekPenaltyDesc_Trace"
            value="ESE DiskOsDeviceSeekPenaltyDesc Trace"
            />
        <string
            id="Event.ESE_DirtyPage2Deprecated_Trace"
            value="ESE DirtyPage2Deprecated Trace"
            />
        <string
            id="Event.ESE_IOCompletion2_Trace"
            value="ESE IOCompletion2 Trace"
            />
        <string
            id="Event.ESE_FCBPurgeFailure_Trace"
            value="ESE FCBPurgeFailure Trace"
            />
        <string
            id="Event.ESE_IOLatencySpikeNotice_Trace"
            value="ESE IOLatencySpikeNotice Trace"
            />
        <string
            id="Event.ESE_IOCompletion2Sess_Trace"
            value="ESE IOCompletion2Sess Trace"
            />
        <string
            id="Event.ESE_IOIssueThreadPost_Trace"
            value="ESE IOIssueThreadPost Trace"
            />
        <string
            id="Event.ESE_IOIssueThreadPosted_Trace"
            value="ESE IOIssueThreadPosted Trace"
            />
        <string
            id="Event.ESE_IOThreadIssueStart_Trace"
            value="ESE IOThreadIssueStart Trace"
            />
        <string
            id="Event.ESE_IOThreadIssuedDisk_Trace"
            value="ESE IOThreadIssuedDisk Trace"
            />
        <string
            id="Event.ESE_IOThreadIssueProcessedIO_Trace"
            value="ESE IOThreadIssueProcessedIO Trace"
            />
        <string
            id="Event.ESE_IOIoreqCompletion_Trace"
            value="ESE IOIoreqCompletion Trace"
            />
        <string
            id="Event.ESE_CacheMemoryUsage_Trace"
            value="ESE CacheMemoryUsage Trace"
            />
        <string
            id="Event.ESE_CacheSetLgposModify_Trace"
            value="ESE CacheSetLgposModify Trace"
            />
        <!--ESE_ETW_AUTOGEN_STRING_LIST_END-->

        <string
          id="Event.ESE_CompressionExperiment_Trace"
          value="ESE Compression Experiment Trace"
          />

        <string id="DirtyLevelMap.bfdfClean" value="bfdfClean"/>
        <string id="DirtyLevelMap.bfdfUntidy" value="bfdfUntidy"/>
        <string id="DirtyLevelMap.bfdfDirty" value="bfdfDirty"/>
        <string id="DirtyLevelMap.bfdfFilthy" value="bfdfFilthy"/>
        <!--ESE_ETW_IORS_STRINGVALUES: Inserted here. See genetw.pl-->
        <!--ESE_ETW_IORT_STRINGVALUES: Inserted here. See genetw.pl-->
        <!--ESE_ETW_IORF_STRINGVALUES: Inserted here. See genetw.pl-->
        <!--ESE_ETW_IORP_STRINGVALUES: Inserted here. See genetw.pl-->
        <string id="PageFlagsMap.fPageRoot" value="fPageRoot"/>
        <string id="PageFlagsMap.fPageLeaf" value="fPageLeaf"/>
        <string id="PageFlagsMap.fPageParentOfLeaf" value="fPageParentOfLeaf"/>
        <string id="PageFlagsMap.fPageEmpty" value="fPageEmpty"/>
        <string id="PageFlagsMap.fPageRepair" value="fPageRepair"/>
        <string id="PageFlagsMap.fPagePrimary" value="fPagePrimary"/>
        <string id="PageFlagsMap.fPageSpaceTree" value="fPageSpaceTree"/>
        <string id="PageFlagsMap.fPageIndex" value="fPageIndex"/>
        <string id="PageFlagsMap.fPageLongValue" value="fPageLongValue"/>
        <string id="PageFlagsMap.fPageNonUniqueKeys" value="fPageNonUniqueKeys"/>
        <string id="PageFlagsMap.fPageNewRecordFormat" value="fPageNewRecordFormat"/>
        <string id="PageFlagsMap.fPageNewChecksumFormat" value="fPageNewChecksumFormat"/>
        <string id="PageFlagsMap.fPageScrubbed" value="fPageScrubbed"/>
        <string id="PageFlagsMap.flushTypeBit1" value="flushTypeBit1"/>
        <string id="PageFlagsMap.flushTypeBit2" value="flushTypeBit2"/>
        <string id="PageFlagsMap.fPagePreInit" value="fPagePreInit"/>
        <!--ESE_ETW_IOFR_STRINGVALUES: Inserted here. See genetw.pl-->
        <!--ESE_ETW_TRACETAG_STRINGVALUES: Inserted here. See genetw.pl-->
        <!--ESE_ETW_JETAPI_STRINGVALUES: Inserted here. See genetw.pl-->
        <!--ESE_ETW_IOFILE_STRINGVALUES: Inserted here. See genetw.pl-->

        <!--ESE_ETW_EVENT_TODO: Optionally add any new string values that help clarify trace elements before this.-->

      </stringTable>
    </resources>
  </localization>
</assembly>
