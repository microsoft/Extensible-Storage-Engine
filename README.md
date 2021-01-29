# Extensible-Storage-Engine

A Non-SQL Database Engine.

The Extensible Storage Engine (ESE, once known as JET Blue) is one of those rare code bases having proven to have a more than 25 year serviceable lifetime.  First shipping in Windows NT 3.51 and shortly thereafter in Exchange 4.0, and rewritten twice in the 90s, and heavily updated over the subsequent two decades after that, it remains a core Microsoft asset to this day running:
- On 100s of thousands of machines and millions of disks for the Office 365 Mailbox Storage Backend servers.
- Large SMP systems with TB of memory for large Active Directory deployments.
- And finally every single Windows Client computer has several database instances running in low memory modes (over 1 billion devices just for Windows 10, but ESE has been in use in Windows client SKUs since Windows XP).

ESE enables applications to store and retrieve data from tables using indexed or sequential cursor navigation. It supports denormalized schemas including wide tables with numerous sparse columns, multi-valued columns, and sparse and rich indexes. It enables applications to enjoy a consistent data state using transacted data update and retrieval. A crash recovery mechanism is provided so that data consistency is maintained even in the event of a system crash. It provides ACID (Atomic Consistent Isolated Durable) transactions over data and schema by way of a write-ahead log and a snapshot isolation model. 
- [Summary of features and the JET API](https://docs.microsoft.com/en-us/windows/win32/extensible-storage-engine/extensible-storage-engine)
- [More extensive list of ESE database features are documented in our Wikipedia entry](https://en.wikipedia.org/wiki/Extensible_Storage_Engine)

However the library provides many other strongly layered and and thus reusable sub-facilities as well:
- A Synchronization / Locking library
- A Data-structures / STL-like library
- An OS-abstraction layer
- A Block / Cache Manager
... as well the full blown database engine itself.

The version of source we post here will likely be a bit in advance of the version compiled into the latest Windows update.  So the JET API documentation may be out of date with it.

## Future Plans

You may notice the initial code is without comments!  The code is so good, it is self evident right?  :-D   We are of course teasing, we are evaluating the comments to clean them of names of individuals that may wish to remain private and/or for tone.  We will be pushing enhanced and cleaned up comments as we are able.  We also will be pushing build project files and a little more infrastructure to get a building ESE.  Right now the code is provided just for instructional purposes only.

## Contributing

Initially our code synchronization process will be one way, so we will be unable to take code contributions.  We will likely be allowing contributions in the future.  If you are interested in contributing, we would like to hear from you.
