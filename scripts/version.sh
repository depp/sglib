#!/bin/sh
set -e
DEFAULT='v0.0-unknown'
SRCDIR="$1"
BUILDDIR="$2"

INPUT="$SRCDIR"/scripts/VERSION
OUTPUT="$BUILDDIR"/src/version.c

if test $# -ne 2
then
    echo 'This should only be called from the Makefile'
	exit 1
fi

if test -f "$INPUT"
then
    # If the release version is present, use that
    VERSION=$(cat "$INPUT") || VERSION="$DEFAULT"
elif test -d "$SRCDIR"/.git
then
    # Otherwise, ask git
    VERSION=$(git --git-dir="$SRCDIR"/.git describe) || VERSION="$DEFAULT"
else
    # As a fallback, use the nonsensical default value
    VERSION="$DEFAULT"
fi

if test -f "$OUTPUT"
then
    OVERSION=$(sed -e 's/^[^"]*"//;s/"[^"]*$//' < "$OUTPUT")
    if test "$VERSION" = "$OVERSION"
    then
        exit 0
    fi
fi

echo "Version $VERSION"
echo "const char VERSION_STRING[] = \"$VERSION\";" >"$OUTPUT".tmp
mv "$OUTPUT".tmp "$OUTPUT"
