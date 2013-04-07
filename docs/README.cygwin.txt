How to build the documentation
==============================
This section explains how to build the German and English HTML-documentation from the English XML-source files and the translation file.

In Linux, it should be as easy as: Open the console and run make in this directory.

In Windows, you need Python and Cygwin. During the Cygwin installation, select these additional libraries to install:
+ make
+ xsltproc (libxml2 + libxslt + python-libxml2)
+ findutils
+ sed
+ gettext
+ gettext-devel
+ libgcrypt + libgpg-error (for whatever reason)

After the installation, run cygwin, cd to this directory (/cygdrive/c/.../openclonk/docs/) and run make. The process takes a few minutes to complete.
The online version of the documentation has been generated into the online/ directory.

To build the .chm files, install the html help compiler from http://www.microsoft.com/en-us/download/details.aspx?id=21138 or http://go.microsoft.com/fwlink/?LinkId=14188 and run make chm HHC=/path/to/hhc.exe
