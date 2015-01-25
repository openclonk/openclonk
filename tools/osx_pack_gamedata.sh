#!/bin/bash

C4GROUP=$1
SRC_GROUP=$2
RESOURCES=${3:-$TARGET_BUILD_DIR/$UNLOCALIZED_RESOURCES_FOLDER_PATH}
TARGET_GROUP=$RESOURCES/`basename $SRC_GROUP`

should_update() {
	for i in `find $SRC_GROUP`; do
		if [ $i -nt $TARGET_GROUP ]; then return 0; fi
	done
	return 1
}

if [ "$CONFIGURATION" == "Debug" ]
then echo Linking $TARGET_GROUP...
	rm -f $TARGET_GROUP
	ln -sf $SRC_GROUP $TARGET_GROUP
else if should_update
	then echo Packing $TARGET_GROUP...
		rm -f $TARGET_GROUP
		cd $RESOURCES
		$C4GROUP $SRC_GROUP -t $TARGET_GROUP
	else echo No changes found for $TARGET_GROUP, skipping
	fi
fi
