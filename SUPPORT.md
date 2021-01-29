# Support

## How to file issues and get help  

At this time, we are not accepting bugs or feature requests here, as we have only JUST launched for reading, not contributing.  That will change once we enable build, and contributions.

For help and questions about using this project, please at this time see the project Wiki tab.

## Regular ESENT Support

If you are using the Windows in-box esent.dll (and Windows SDK esent.h) for your application and hitting an issue, you should try the regular Windows Developer support channels.

## Public vs. Private JET API

If you are building off this project, keep in mind ...

The jethdr.w file is our main ESE / JET API header, but it contains what we consider to be private JET APIs.  You will note it is similar to the esent.h header in the Windows SDK.  The Windows build process splits jethdr.w into public esent.h and a private version.  The functions, constants and definitions in between these tags:
// end_PubEsent
// begin_PubEsent
are private JET APIs.

We have to include jethdr.w because ESE needs it to build.  But we do not support applications using the private constants / arguments and JET APIs.  These constants / APIs are often only used by ESE Tests, or a single special Microsoft internal client that we have code control over.  The private constants and APIs are therefore NOT stable APIs or behaviors that your application can depend upon.  We are free to change the behavior of such APIs at will, and without concern how it may affect your application.

That caveat aside, our public API (such as you see in SDK esent.h) we consider to be very stable.  And would only follow longer Microsoft deprecation guidelines if we were to change it.  You can write your application knowing we likely will not break you if you just use esent.h (instead of jethdr.w).

Once we allow contributions, we will have to provide this github repository some version of hdrsplit (Header Split) to create an esent.h out of your potentially modified jethdr.w.


## Microsoft Support Policy  

Support for this **PROJECT or PRODUCT** is limited to the resources listed above.
