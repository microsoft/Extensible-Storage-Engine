# Support

## How to file issues and get help

At this time, we are not accepting bugs or feature requests here, as we have only JUST launched for reading, not contributing.  That will change once we enable build/test and begin accepting contributions.

For help and questions about using this project, please at this time see the project Wiki tab.

## Regular ESENT Support

If you are using the Windows in-box `esent.dll` (and Windows SDK `esent.h`) for your application and hitting an issue, you should try the regular Windows Developer support channels, as before.

## Public vs. Private JET API

You may notice that `jethdr.w` looks a lot like Windows SDK 'esent.h', but you should note it contains what we consider to be private JET APIs.  You should not use jethdr.w as esent.h, as we **will not provide support for applications using the private constants, private arguments, and private JET APIs**.  These constants / APIs are often only used by ESE tests or a single special Microsoft internal client that we have code control over.  The private constants and APIs are therefore NOT stable APIs or behaviors that your application can depend upon.  We are free to change the behavior of such APIs at will, and without concern for how it may affect your application.

That caveat aside, our public APIs (such as what you see in the SDK `esent.h`) we consider to be very stable.  These APIs would follow the longer Microsoft deprecation guidelines even if we were to change them.  You can write your application knowing we likely will not break you if you just use `esent.h` instead of `jethdr.w`.

## Microsoft Support Policy

Support for this **PROJECT or PRODUCT** is limited to the resources listed above.
