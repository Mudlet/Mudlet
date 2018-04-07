These project files were tested with the 
free Microsoft Visual C++ 2005 Express Edition.

Visual C++ Express does NOT include the Win32 SDK Headers.
They must be downloaded separately as documented in the
installation instructions at Microsoft - consequently that
extra installation must be referenced via a system variable:
Set MSSdk=C:\Program Files\Microsoft Platform SDK

In order to compile you need a valid Zlib SDK - you can
download it from zlib.org in the archive zlib123-dll.zip -
I had unpacked them to $(zziplibdir)/zlib inside the
toplevel of the zziplib source tree. That SDK does ship
with a "zlib1.dll" runtime and "zdll.lib" import library.

The new Visual Studio has depracated most of the Win32 API
just as well as most of the Posix API. That provokes a lot
of warning message in surplus of the many warning message
that occur on every implicit conversion between signed and
unsigned types. Other compilers are more clever about
detecting real conversion issues, therefore they are not
going to be "patched away" with an explicit conversion in
the source files (I expect Microsoft to get it right somewhen).

== Download Places ==
http://www.microsoft.com/msdownload/platformsdk/sdkupdate/ 
= central place referenced a lot in the MSDN but the link is broken

http://download.microsoft.com/download/a/5/f/a5f0d781-e201-4ab6-8c6a-9bb4efed1e1a/PSDK-x86.exe 
http://download.microsoft.com/download/a/5/f/a5f0d781-e201-4ab6-8c6a-9bb4efed1e1a/PSDK-amd64.exe 
http://download.microsoft.com/download/a/5/f/a5f0d781-e201-4ab6-8c6a-9bb4efed1e1a/PSDK-ia64.exe 
= Windows Server 2003 SP1 Platform SDK Web Install (win32/win64/.. - but SDK for all CPU types)

http://download.microsoft.com/download/f/a/d/fad9efde-8627-4e7a-8812-c351ba099151/PSDK-x86.exe
http://download.microsoft.com/download/f/a/d/fad9efde-8627-4e7a-8812-c351ba099151/PSDK-ia64.exe
http://download.microsoft.com/download/f/a/d/fad9efde-8627-4e7a-8812-c351ba099151/PSDK-amd64.exe
= Windows Server 2003 R2 Platform SDK Web Install (win32/win64/.. - but SDK for all CPU types)

http://download.microsoft.com/download/A/9/1/A91D6B2B-A798-47DF-9C7E-A97854B7DD18/VC.iso 
= Visual C++ Express CD-image
http://download.microsoft.com/download/C/6/F/C6F4733B-67C7-4C15-9F21-61F7A3167505/VCS.iso 
= Visual C# Express CD-image

http://zlib.net/zlib123-dll.zip (broken)
http://zlib.net/zlib125-dll.zip
= unpack one of them into zzip-0/zlib (if using the subversion checkout)

== Environment Variables ==
On Windows-7 the environment variables can be found in the
control panel > system information > change name settings > extras tab > click button

== Debug Variables ==
To run the zziptest for debugging the solution file has now a setting
in Properties > Debugging > Environment > PATH=$(InputDir)\..\zlib\
so that the zlib1.dll can be found. This needs to be adapted if the
zlib123-dll.zip was unpackaged somewhere else.

== Compiling Error ==
'can not delete debug/*.idb' - occurs when running MSVC in a VMWare box 
pointing to a Unix drive; create a symlink `ln -s Debug debug` to fix it.
There are Pre/Post-Build Rules to remove *.idb but sometimes its left over.
