#!/usr/bin/env bash

error() {
	echo error: "$@"
	exit 1
}

[[ -f CMakeCache.txt ]] || error "this does not look like a build directory"

DESTDIR=appdir cmake --build . --target install || error "install failed"

linuxdeployqt=linuxdeployqt-continuous-x86_64.AppImage
if [[ ! -x $linuxdeployqt ]]; then
	wget -c -nv "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/$linuxdeployqt" \
		|| error "could not download linuxdeployqt"
	chmod a+x $linuxdeployqt
fi

export ARCH=$(uname -m)

./$linuxdeployqt appdir/usr/share/applications/openclonk.desktop -appimage \
	|| error "AppImage creation has failed"
