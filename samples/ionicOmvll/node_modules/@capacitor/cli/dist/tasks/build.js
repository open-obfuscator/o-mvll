"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.build = exports.buildCommand = void 0;
const build_1 = require("../android/build");
const common_1 = require("../common");
const errors_1 = require("../errors");
const build_2 = require("../ios/build");
async function buildCommand(config, selectedPlatformName, buildOptions) {
    var _a;
    const platforms = await (0, common_1.selectPlatforms)(config, selectedPlatformName);
    let platformName;
    if (platforms.length === 1) {
        platformName = platforms[0];
    }
    else {
        platformName = await (0, common_1.promptForPlatform)(platforms.filter(createBuildablePlatformFilter(config)), `Please choose a platform to build for:`);
    }
    const buildCommandOptions = {
        scheme: buildOptions.scheme || config.ios.scheme,
        flavor: buildOptions.flavor || config.android.flavor,
        keystorepath: buildOptions.keystorepath || config.android.buildOptions.keystorePath,
        keystorepass: buildOptions.keystorepass || config.android.buildOptions.keystorePassword,
        keystorealias: buildOptions.keystorealias || config.android.buildOptions.keystoreAlias,
        keystorealiaspass: buildOptions.keystorealiaspass ||
            config.android.buildOptions.keystoreAliasPassword,
        androidreleasetype: buildOptions.androidreleasetype ||
            config.android.buildOptions.releaseType ||
            'AAB',
        signingtype: buildOptions.signingtype ||
            config.android.buildOptions.signingType ||
            'jarsigner',
        configuration: buildOptions.configuration || 'Release',
    };
    try {
        await build(config, platformName, buildCommandOptions);
    }
    catch (e) {
        if (!(0, errors_1.isFatal)(e)) {
            (0, errors_1.fatal)((_a = e.stack) !== null && _a !== void 0 ? _a : e);
        }
        throw e;
    }
}
exports.buildCommand = buildCommand;
async function build(config, platformName, buildOptions) {
    if (platformName == config.ios.name) {
        await (0, build_2.buildiOS)(config, buildOptions);
    }
    else if (platformName === config.android.name) {
        await (0, build_1.buildAndroid)(config, buildOptions);
    }
    else if (platformName === config.web.name) {
        throw `Platform "${platformName}" is not available in the build command.`;
    }
    else {
        throw `Platform "${platformName}" is not valid.`;
    }
}
exports.build = build;
function createBuildablePlatformFilter(config) {
    return platform => platform === config.ios.name || platform === config.android.name;
}
