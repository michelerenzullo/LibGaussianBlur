#!/bin/sh

# Exit on errors
set -e

# Check for required arguments
if [ "$#" -lt 3 ]; then
  echo "Usage: $0 <GH_TOKEN> <GH_REPO> <ACTION> [--prefix <PREFIX>] [--exclude <EXCLUDE_EXACT>]"
  echo "ACTION must be either '--old' or '--duplicates'."
  exit 1
fi

GH_TOKEN=$1
GH_REPO=$2
ACTION=$3
shift 3  # Shift to process optional arguments

PREFIX=""
EXCLUDE_EXACT=""

# Parse optional arguments
while [ "$#" -gt 0 ]; do
  case "$1" in
    --prefix)
      PREFIX=$2
      shift 2
      ;;
    --exclude)
      EXCLUDE_EXACT=$2
      shift 2
      ;;
    *)
      echo "Unknown argument: $1"
      exit 1
      ;;
  esac
done

# Configure GitHub CLI authentication
export GH_TOKEN
export GH_REPO

echo "ACTION: $ACTION"
echo "PREFIX: $PREFIX"
echo "EXCLUDE_EXACT: $EXCLUDE_EXACT"

if [ "$ACTION" = "--old" ]; then
  if [ -z "$PREFIX" ] || [ -z "$EXCLUDE_EXACT" ]; then
    echo "For --old, both --prefix and --exclude must be provided."
    exit 1
  fi

  echo "Delete caches with prefix ${PREFIX}, excluding exact match ${EXCLUDE_EXACT}"
  # Fetch and filter the list of caches
  cache_list=$(gh cache list --json key,id --jq \
    'map({key: .key | select(startswith("'${PREFIX}'") and . != "'${EXCLUDE_EXACT}'"), id: .id}) | .[].id' --limit 1000)

  # Print the list of caches to be deleted
  echo "Duplicate caches to be deleted:"
  if [ -z "$cache_list" ]; then
    echo "None"
  else
    echo "$cache_list"
    # Delete the caches
    echo "$cache_list" | xargs -I{} gh cache delete {}
    echo "Done"
  fi

elif [ "$ACTION" = "--duplicates" ]; then
  echo "Delete duplicate keys if any (the first with newer lastAccessedAt will be kept)"
  # Fetch and filter the list of duplicates
  duplicate_list=$(gh cache list --json key,id,lastAccessedAt --limit 1000 | jq -r '
    map({key: .key, id: .id, lastAccessedAt: .lastAccessedAt}) |
    group_by(.key) |
    map(
      if length > 1 then
        sort_by(.lastAccessedAt) | .[:-1] | .[]
      else
        empty
      end
    ) | .[].id')

  # Print the list of duplicates to be deleted
  echo "Duplicate caches to be deleted:"
  if [ -z "$duplicate_list" ]; then
    echo "None"
  else
    echo "$duplicate_list"
    # Delete the duplicate caches
    echo "$duplicate_list" | xargs -I{} gh cache delete {}
    echo "Done"
  fi
else
  echo "Invalid ACTION: $ACTION"
  echo "Use either '--old' or '--duplicates'."
  exit 1
fi