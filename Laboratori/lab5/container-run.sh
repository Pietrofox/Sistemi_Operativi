#!/bin/bash

# Check if correct number of arguments is given
if [ "$#" -lt 2 ]; then
    echo "Usage: $0 conf-file command [args...]"
    exit 1
fi

# Read arguments
CONF_FILE="$1"
shift
#COMMAND="$@"

# Create a temporary working directory
WORKDIR=$(mktemp -d)
echo "Working directory created at: $WORKDIR"

# Function to clean up the working directory on exit
cleanup() {
    echo "Cleaning up working directory..."
    rm -rf "$WORKDIR"
}
trap cleanup EXIT

# Parse the configuration file and set up the environment
while IFS=$'\n' read -r SRC DST; do
    if [ -z "$SRC" ] || [ -z "$DST" ]; then
        continue
    fi
    # Create destination directory
    mkdir -p "$WORKDIR/$(dirname "$DST")"
    if [ -d "$SRC" ]; then
        # Bind mount directories
        bindfs --no-allow-other "$SRC" "$WORKDIR$DST"
    else
        # Copy files
        cp "$SRC" "$WORKDIR$DST"
    fi
done < "$CONF_FILE"

# Run the command in the isolated environment using fakechroot
fakechroot chroot "$WORKDIR"
