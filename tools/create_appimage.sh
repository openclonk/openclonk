#!/usr/bin/env bash

error() {
	echo error: "$@"
	exit 1
}

[[ -f CMakeCache.txt ]] || error "this does not look like a build directory"

# Read current branch from the CMake-generated C4Version.h.
branch=$(cpp -dM C4Version.h | awk '$2 == "C4REVISION_BRANCH" { print gensub("^\"|\"$", "", "g", $3) }')
[[ -n $branch ]] || error "could not get branch name"

DESTDIR=appdir cmake --build . --target install || error "install failed"

linuxdeployqt=linuxdeployqt-continuous-x86_64.AppImage
if [[ ! -x $linuxdeployqt ]]; then
	wget -c -nv "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/$linuxdeployqt" \
		|| error "could not download linuxdeployqt"
	chmod a+x $linuxdeployqt
fi

export ARCH=$(uname -m)

# We need to run appimagetool manually to be able to specify an update URL.
coproc { exec stdbuf -oL ./$linuxdeployqt --appimage-mount; }
trap "kill -SIGINT $COPROC_PID" EXIT
read -r -u "${COPROC[0]}" mount_path
PATH="$mount_path/usr/bin:$PATH"

linuxdeployqt appdir/usr/share/applications/openclonk.desktop -bundle-non-qt-libs \
	|| error "linuxdeployqt has failed"

appimagetool appdir --verbose -n -u "zsync|https://releases.openclonk.org/snapshots/latest-$branch/OpenClonk-x86_64.AppImage.zsync" \
	|| error "appimagetool has failed"
