#!/bin/sh
aclocal -I autotools --install
autoheader
autoconf
automake -a

