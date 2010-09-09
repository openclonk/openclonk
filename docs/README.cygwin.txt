How to build the documentation
==============================
This section explains how to build the German and English HTML-documentation from the English XML-source files and the translation file.

In Linux, it should be as easy as: Open the console and run make in this directory.

In Windows, you ned Python and Cygwin. During the Cygwin installation, select these additional libraries to install:
+ make
+ xsltproc (libxml2 + libxslt + python-libxml2)
+ findutils
+ sed
+ gettext
+ gettext-devel
+ libgcrypt + libgpg-error (for whatever reason)
Also you might have to rename find.exe, so that the cygwin provided one is used - first in Windows\System32\dllback then in Windows\System32 to make sure it isn't used.

After the installation, run cygwin, cd to this directory (/cygdrive/c/.../openclonk/docs/) and run make. The process takes about ten minutes to complete.
The online version of the documentation has been generated into the online/ directory.