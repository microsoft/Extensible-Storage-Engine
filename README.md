# Extensible-Storage-Engine

## A Non-SQL Database Engine

The Extensible Storage Engine (ESE) is one of those rare codebases having proven to have a more than 25 year serviceable lifetime.  First shipping in Windows NT 3.51 and shortly thereafter in Exchange 4.0, and rewritten twice in the 90s, and heavily updated over the subsequent two decades after that, it remains a core Microsoft asset to this day.

- It's running on 100s of thousands of machines and millions of disks for the Office 365 Mailbox Storage Backend servers
- It's also running on large SMP systems with TB of memory for large Active Directory deployments
- Every single Windows Client computer has several database instances running in low memory modes. In over 1 billion Windows 10 devices today, ESE has been in use in Windows client SKUs since Windows XP

ESE enables applications to store data to, and retrieve data from tables using indexed or sequential cursor navigation.  It supports denormalized schemas including wide tables with numerous sparse columns, multi-valued columns, and sparse and rich indexes.  ESE enables applications to enjoy a consistent data state using transacted data update and retrieval.  A crash recovery mechanism is provided so that data consistency is maintained even in the event of a system crash.  ESE provides ACID (Atomic Consistent Isolated Durable) transactions over data and schema by way of a write-ahead log and a snapshot isolation model.

- A summary of features and the JET API documentation are up on [Microsoft's official documentation site](https://docs.microsoft.com/en-us/windows/win32/extensible-storage-engine/extensible-storage-engine)
- A more extensive list of ESE database features [are documented in our Wikipedia entry](https://en.wikipedia.org/wiki/Extensible_Storage_Engine)

The library provides many other strongly layered and, thus, reusable sub-facilities as well:
- A synchronization and locking library
- An STL-like data structures library
- An OS abstraction layer
- A Block / Cache Manager

All this is in addition to the full-blown database engine itself.

The version of source we post here will likely be a bit in advance of the version compiled into the latest Windows update.  Therefore, the JET API documentation may be out of date with it.

## Is this the JET database / engine?

No.  Well ... it depends ... the question is not quite correct.  Most people do not know that JET was an acronym for an API set, not a specific database format or engine.  Just as there is no such thing as "the SQL engine", as there are many implementations of the protocol, there is no "JET engine" or "JET database".  It is in the acronym, "Joint Engine Technology".  And as such, there are two separate implementations of the JET API.  This is the JET Blue engine implementation, see [Notes in here](https://docs.microsoft.com/en-us/windows/win32/extensible-storage-engine/extensible-storage-engine).  The origin of the [colors have an an amusing source](https://en.wikipedia.org/wiki/Extensible_Storage_Engine#History) by the way.  Most people think of the "JET engine" as JET Red, that shipped under Microsoft Access.  This is not that "JET engine".  We renamed to ESE to try to avoid this confusion, but it seems that the confusion continues to this day.

## Future Plans

### Comments

You may notice the initial code is without comments!  This codebase has a long history of internal development at Microsoft, so, in order to stay on the safe side with the very first release of the source code, we have temporarily removed all comments and excluded certain file types.  We will be pushing enhanced and cleaned up comments as we are able to review them.

### CMake

We also will be pushing build files, codegen scripts, and a little more infrastructure to get a building ESE.  Right now, the code is provided just for instructional purposes only.

### Tests

We are initially withholding the test code, and, as with the comments and the codegen scripts, we will be gradually releasing the tests, as well as adding Azure pipelines to run them.

## Contributing

Initially, our code synchronization process will be one way, so we will be unable to take code contributions.  We will be allowing contributions in the future.  If you are interested in contributing, we would like to hear from you!
