#!/bin/bash

cd ${0%/*}

SAMPLES_DIRECTORY=$(realpath ../samples/)
PATCH_ABSOLUTE_FOLDER="${SAMPLES_DIRECTORY}/patches"


# Kotoba sample
KOTOBA_NAME="Kotoba"
KOTOBA_REPO_URL="https://github.com/willhains/Kotoba"
KOTOBA_PATCH_FILE="${PATCH_ABSOLUTE_FOLDER}/Kotoba.patch"
KOTOBA_REPO_FOLDER="${SAMPLES_DIRECTORY}/$KOTOBA_NAME"

# GoodNight sample
GOODNIGHT_NAME="GoodNight"
GOODNIGHT_REPO_URL="https://github.com/anthonya1999/GoodNight"
GOODNIGHT_PATCH_FILE="${PATCH_ABSOLUTE_FOLDER}/GoodNight.patch"
GOODNIGHT_REPO_FOLDER="${SAMPLES_DIRECTORY}/$GOODNIGHT_NAME"

# HackerNews sample
HACKERNEWS_NAME="HackerNews"
HACKERNEWS_REPO_URL="https://github.com/amitburst/HackerNews"
HACKERNEWS_PATCH_FILE="${PATCH_ABSOLUTE_FOLDER}/HackerNews.patch"
HACKERNEWS_REPO_FOLDER="${SAMPLES_DIRECTORY}/$HACKERNEWS_NAME"

# HexaCalc sample
HEXACALC_NAME="HexaCalc"
HEXACALC_REPO_URL="https://github.com/AnthonyH93/HexaCalc"
HEXACALC_PATCH_FILE="${PATCH_ABSOLUTE_FOLDER}/HexaCalc.patch"
HEXACALC_REPO_FOLDER="${SAMPLES_DIRECTORY}/$HEXACALC_NAME"

# SnowPlow sample
SNOWPLOW_NAME="SnowPlow"
SNOWPLOW_REPO_URL="https://github.com/snowplow/snowplow-ios-tracker"
SNOWPLOW_PATCH_FILE="${PATCH_ABSOLUTE_FOLDER}/SnowPlowExamples.patch"
SNOWPLOW_REPO_FOLDER="${SAMPLES_DIRECTORY}/$SNOWPLOW_NAME"

function test {
    "$@"
    local status=$?
    if [ $status -ne 0 ]; then
        if [ "$1" != "/bin/false" ]; then
            echo "${bold}ERROR in line ${BASH_LINENO[0]}: Command '$*' failed with exit code $status${normal}"
        else
            echo "${bold}ERROR in line ${BASH_LINENO[0]} in file $0${normal}"
        fi
        exit -1
    fi
    return $status
}

function getRepo() {
    local outputFolder=$1
    local repoUrl=$2
    rm -rf $outputFolder
    test git clone --recurse-submodules $repoUrl $outputFolder
}

function applyPatch() {
    local folder=$1
    local patchPath=$2
    cd $folder
    test git apply $patchPath
    cd -
}

function getKotobaSample() {
    getRepo ${KOTOBA_REPO_FOLDER} ${KOTOBA_REPO_URL}
    applyPatch ${KOTOBA_REPO_FOLDER} ${KOTOBA_PATCH_FILE}
}

function getGoodNightSample() {
    getRepo ${GOODNIGHT_REPO_FOLDER} ${GOODNIGHT_REPO_URL}
    applyPatch ${GOODNIGHT_REPO_FOLDER} ${GOODNIGHT_PATCH_FILE}
}

function getHackerNewsSample() {
    getRepo ${HACKERNEWS_REPO_FOLDER} ${HACKERNEWS_REPO_URL}
    applyPatch ${HACKERNEWS_REPO_FOLDER} ${HACKERNEWS_PATCH_FILE}
    cd ${HACKERNEWS_REPO_FOLDER}
    test pod install
    cd -
}

function getHexaCalcSample() {
    getRepo ${HEXACALC_REPO_FOLDER} ${HEXACALC_REPO_URL}
    applyPatch ${HEXACALC_REPO_FOLDER} ${HEXACALC_PATCH_FILE}
}

function getSnowPlow() {
    getRepo ${SNOWPLOW_REPO_FOLDER} ${SNOWPLOW_REPO_URL}
    applyPatch ${SNOWPLOW_REPO_FOLDER}/Examples ${SNOWPLOW_PATCH_FILE}
    # Prepare Pods
    cd ${SNOWPLOW_REPO_FOLDER}/Examples/demo/SnowplowObjcDemo
    test pod install
    cd -
}

# This is something required by Jenkins for some reason it is not the default one
export LANG=en_US.UTF-8

# Start download dependencies
getGoodNightSample
getHackerNewsSample
getHexaCalcSample
getSnowPlow
