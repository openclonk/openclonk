#!/bin/sh

# Removed, I think those should exist on every Mac Os system: z|png[0-9]*|iconv
LIBS_TO_BUNDLE=".*?/lib(jpeg|GLEW|llvm|SDL|SDL_mixer|freetype|ogg|vorbis|vorbisfile)\.[^ ]+\.dylib"

cd $TARGET_BUILD_DIR
echo "Bundling libraries..."

function bundle () {
	
	for lib in `otool -L $1 | grep -Eo "$LIBS_TO_BUNDLE" | grep -v "@executable_path.*"`; do

		base=`basename $lib`
		mkdir -p $FRAMEWORKS_FOLDER_PATH
		bundle_path=$FRAMEWORKS_FOLDER_PATH/$base

		echo Changing search path for $base in $1
		id=@executable_path/../Frameworks/$base
		install_name_tool -change $lib $id $1

		if [ ! -e $bundle_path ]; then 
			echo Bundling $lib from $bundle_path
			cp $lib $bundle_path
			chmod u+w $bundle_path
			install_name_tool -id $id $bundle_path

			bundle $bundle_path
		fi

	done
}

bundle $EXECUTABLE_PATH
