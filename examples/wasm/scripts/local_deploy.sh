#!/bin/sh

OS=$(uname)

help() {
echo "Utility for deploying a local version of the platform using lighttpd

USAGE
    $0 [OPTIONS]

OPTIONS
    -h, --help  Show this message and exit.
"
}

# Recursively iterates upwards in file folder structure until
# either a git-root directory is found, or it cannot move further up
git_root() {
    git rev-parse --show-toplevel
}

echo_error() {
    printf "ERROR: %s" "$@" 1>&2
}

parse_arguments() {
    while test $# -gt 0; do
        case "$1" in
            -h|--help)
                help ; exit 0 ;;
            -*)
                echo_error "Invalid flag: $1"
                exit 1
                ;;
        esac
    done
}

main() {
    parse_arguments "$@"

    root_path=$(git_root)
    cd "$root_path" || { echo_error "Failed to change directory to git root, aborting."; exit 1; }

    config_template="$root_path/examples/wasm/scripts/local-lighttpd-template.conf"
    config_file="$root_path/examples/wasm/scripts/local-lighttpd-config.conf"
    build_dir="$root_path/external/wasm/wasm32/bin"

    # Escapes forward slashes and spaces for sed, might need
    # adjusting if anyone has paths with more funky characters
    escaped_build_dir=$(echo "$build_dir" | sed -E 's/([/ ])/\\\1/g')

    # Create config file with overwritten document-root and event-handler properties
    if [ "$OS" = "Darwin" ]; then
        # macOS use different system handler than "linux-sysepoll"
        sed -E -e "s/^(server.document-root).*/\1 = \"$escaped_build_dir\"/" -e "s/^(server.event-handler).*/\1 = \"freebsd-kqueue\"/" < "$config_template" > "$config_file"
    else
        sed -E "s/^(server.document-root).*/\1 = \"$escaped_build_dir\"/" < "$config_template" > "$config_file"
    fi

    # Check that config is valid
    if ! lighttpd -tt -f "$config_file" ; then
        echo_error "Invalid config generated, aborting."; exit 1;
    fi

    echo "Starting server at http://localhost:8080"

    lighttpd -D -f "$config_file"
}

main "$@"
