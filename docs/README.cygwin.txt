To build the doku under Windows, you need 

Python and Cygwin 

installed with:

make
xsltproc (libxml2 + libxslt + python-libxml2)
findutils
sed
gettext
gettext-devel
libgcrypt + libgpg-error (for whatever reason)

Also you might have to rename find.exe, so that the cygwin provided one is used - first in Windows\System32\dllback then in Windows\System32 to make sure it isn't used.



Alternative method with MSys (not really recommended, far too complicated)
http://sourceforge.net/projects/mingw/files/MSYS%20Base%20System/msys-1.0.11/MSYS-1.0.11.exe/download
libxslt from http://www.zlatkovic.com/libxml.en.html
http://python.org/download/ (Python 2.6.4 Windows installer works)
http://users.skynet.be/sbi/libxml-python/ (libxml2-python-2.7.4.win32-py2.6.exe works)
http://sourceforge.net/projects/mingw/files/MinGW gettext/gettext-0.17-1/gettext-0.17-1-mingw32-dev.tar.lzma/download
http://sourceforge.net/projects/mingw/files/MinGW gettext/gettext-0.17-1/libgettextpo-0.17-1-mingw32-dll-0.tar.lzma/download
http://sourceforge.net/projects/mingw/files/MinGW gettext/gettext-0.17-1/libintl-0.17-1-mingw32-dll-8.tar.lzma/download
http://sourceforge.net/projects/mingw/files/MinGW libiconv/libiconv-1.13.1-1/libiconv-1.13.1-1-mingw32-dll-2.tar.lzma/download
http://sourceforge.net/projects/mingw/files/MinGW libiconv/libiconv-1.13.1-1/libiconv-1.13.1-1-mingw32-dll-2.tar.lzma/download
(optional) http://go.microsoft.com/fwlink/?LinkId=14188 or http://www.microsoft.com/downloads/details.aspx?displaylang=en&FamilyID=00535334-c8a6-452f-9aa0-d597d16580cc