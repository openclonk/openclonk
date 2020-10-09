#!/usr/bin/env bash

error() {
	echo error: "$@"
	exit 1
}

revision=$(git rev-parse --short=9 HEAD)
[[ -n $revision ]] || error "could not get git revision"

branch=$(git symbolic-ref --short HEAD)
# Travis checks out a concrete revision, so we have to get the branch name from
# the environment instead.
: ${branch:=$TRAVIS_BRANCH}
[[ -n $branch ]] || error "could not get branch name"

date=$(TZ=UTC git show -s --format=%cd --date=format-local:%Y-%m-%dT%H:%M:%SZ)
[[ -n $date ]] || error "could not get commit date"

: ${OC_REL_URL:?target URL not set}
(($# > 0)) || error "no files to upload given"

for file in "$@"; do
	upload_path="/snapshots/$date-$branch-$revision/$(basename "$file")"
	echo "uploading $upload_path"

	curl -XPOST "$OC_REL_URL$upload_path" --data-binary "@$file" \
		|| error "file upload failed"
done
