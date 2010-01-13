To build the doku under Windows, you need 

Perl, Python, and Cygwin 

installed with:

make
xsltproc (libxml2 + libxslt + python-libxml2)
findutils
sed
gettext
gettext-devel
libgcrypt + libgpg-error (for whatever reason)

Also you should rename find.exe - first in Windows\System32\dllback then in Windows\System32 to make sure it isn't used.