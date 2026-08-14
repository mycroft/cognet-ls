#!/bin/sh
# Test suite for the ls clone. Run from the repo root: ./test.sh

set -u

BIN=./ls
DIR=$(mktemp -d) || exit 1
trap 'rm -rf "$DIR"' EXIT INT TERM

fail=0

ok() { printf 'ok   %s\n' "$1"; }
bad() {
    printf 'FAIL %s\n' "$1"
    [ "$#" -gt 1 ] && printf '  %s\n' "$2"
    fail=1
}

check_eq() { # <desc> <expected> <actual>
    if [ "$2" = "$3" ]; then
        ok "$1"
    else
        bad "$1" "expected: $(printf '%s' "$2" | tr '\n' '|')
  actual:   $(printf '%s' "$3" | tr '\n' '|')"
    fi
}

check_contains() { # <desc> <needle> <haystack>
    case "$3" in
        *"$2"*) ok "$1" ;;
        *) bad "$1" "missing: $2" ;;
    esac
}

line_of() { # <name> <listing> -> 1-based line number, empty if absent
    printf '%s\n' "$2" | grep -n -x "$1" | head -1 | cut -d: -f1
}

# --- fixture (explicit modes so permission checks are deterministic) ---
touch "$DIR/alpha" "$DIR/beta" "$DIR/.hidden"
mkdir "$DIR/sub"
touch "$DIR/sub/gamma"
ln -s alpha "$DIR/link"
touch -t 202001010000 "$DIR/old"
touch -t 202106150000 "$DIR/mid"
chmod 755 "$DIR" "$DIR/sub"
chmod 644 "$DIR/alpha" "$DIR/beta" "$DIR/.hidden" "$DIR/old" "$DIR/mid" "$DIR/sub/gamma"

# --- default listing ---
check_eq "default listing, alpha order" \
    "alpha
beta
link
mid
old
sub" \
    "$($BIN "$DIR")"

# --- -a ---
check_eq "-a includes dotfiles" \
    ".
..
.hidden
alpha
beta
link
mid
old
sub" \
    "$($BIN -a "$DIR")"

# --- -l ---
long=$($BIN -l "$DIR")
check_contains "-l: regular file perms" "-rw-r--r--" "$long"
check_contains "-l: directory type char" "drwxr-xr-x" "$long"
check_contains "-l: symlink type char" "lrwxrwxrwx" "$long"

# --- -t / -tr ---
t=$($BIN -t "$DIR")
mid_ln=$(line_of mid "$t")
old_ln=$(line_of old "$t")
if [ -n "$mid_ln" ] && [ -n "$old_ln" ] && [ "$mid_ln" -lt "$old_ln" ]; then
    ok "-t: newest first (mid before old)"
else
    bad "-t: newest first (mid before old)" "mid=$mid_ln old=$old_ln"
fi

tr_out=$($BIN -tr "$DIR")
mid_ln=$(line_of mid "$tr_out")
old_ln=$(line_of old "$tr_out")
if [ -n "$mid_ln" ] && [ -n "$old_ln" ] && [ "$mid_ln" -gt "$old_ln" ]; then
    ok "-tr: oldest first (old before mid)"
else
    bad "-tr: oldest first (old before mid)" "mid=$mid_ln old=$old_ln"
fi

# --- -r ---
check_eq "-r reverses alpha order" \
    "sub
old
mid
link
beta
alpha" \
    "$($BIN -r "$DIR")"

# --- -R ---
r=$($BIN -R "$DIR")
check_contains "-R: subdir marked with slash" "sub/" "$r"
check_contains "-R: recursed into subdir" "gamma" "$r"

# --- -l on a symlink argument ---
lnk=$($BIN -l "$DIR/link")
check_contains "-l symlink arg: shows link itself" "lrwxrwxrwx" "$lnk"
check_contains "-l symlink arg: shows target" "-> alpha" "$lnk"
check_contains "-l: symlink target shown" "link -> alpha" "$long"

# --- error handling ---
$BIN /nonexistent-path-xyz 2>/dev/null
check_eq "nonexistent path exits 1" "1" "$?"
$BIN -x 2>/dev/null
check_eq "invalid option exits 2" "2" "$?"

# --- multiple arguments ---
multi=$($BIN "$DIR/alpha" "$DIR/sub")
check_contains "multi-arg: path header" "$DIR/sub:" "$multi"
check_contains "multi-arg: file listed" "alpha" "$multi"

if [ "$fail" -eq 0 ]; then
    echo "all tests passed"
else
    echo "tests failed"
fi
exit "$fail"
