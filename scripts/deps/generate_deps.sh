#!/usr/bin/env bash
set -e

SCRIPT_PATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
platform=""

task_print_usage() {
    echo "Usage: generate_deps.sh -p [platform]"
    echo "Example: generate_deps.sh -p xcode"
    echo ""
    echo ""
    echo "------------------------------------------------------------------------------"
    echo "Platform:"
    echo "  choose a platform with -p | --platform <target>"
    echo "      ndk                   Build deps for ndk environment"
    echo ""
    echo ""
}

generate_deps() {
    # Generate dependencies per platform
    echo "Generate DEPS folder for platform $platform"
    rm -rf tmp && mkdir tmp
    cd tmp
    mkdir omvll-deps
    # Generate common elements
    ${SCRIPT_PATH}/common/compile_cpython310.sh
    ${SCRIPT_PATH}/common/compile_pybind11.sh
    ${SCRIPT_PATH}/common/compile_spdlog.sh
    if [ "$platform" == "ndk" ]; then
        ${SCRIPT_PATH}/ndk/compile_llvm_r26d.sh
        # Generate tar
        mv omvll-deps omvll-deps-ndk-r26d
        tar -cvf ../omvll-deps-ndk-r26d.tar omvll-deps-ndk-r26d
    fi
    cd ..
    rm -rf tmp
}

while [[ $# -gt 0 ]]
do
key="$1"

case $key in
    -h | --help )   task_print_usage
                    exit 0
                    ;;
    -p | --platform )
                    case $2 in
                        ndk ) if [ "$platform" != "" ]; then
                                  task_print_usage
                                  exit 1
                              fi
                              platform="$2"
                        ;;
                    esac
                    shift # past argument
                    shift # past value
                    ;;
    *)    # unknown option
    shift # past argument
    ;;
esac
done

if [ $platform != "ndk" ]; then
   echo "Please specify a valid --platform. For more information use --help." >&2
   exit 1
fi

generate_deps
rc=$?
exit $rc
