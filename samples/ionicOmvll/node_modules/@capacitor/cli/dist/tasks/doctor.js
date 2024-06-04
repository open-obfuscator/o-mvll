"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.doctor = exports.doctorCore = exports.doctorCommand = void 0;
const tslib_1 = require("tslib");
const utils_fs_1 = require("@ionic/utils-fs");
const doctor_1 = require("../android/doctor");
const colors_1 = tslib_1.__importDefault(require("../colors"));
const common_1 = require("../common");
const doctor_2 = require("../ios/doctor");
const log_1 = require("../log");
const emoji_1 = require("../util/emoji");
const node_1 = require("../util/node");
const subprocess_1 = require("../util/subprocess");
async function doctorCommand(config, selectedPlatformName) {
    log_1.output.write(`${(0, emoji_1.emoji)('💊', '')}   ${colors_1.default.strong('Capacitor Doctor')}  ${(0, emoji_1.emoji)('💊', '')} \n\n`);
    await doctorCore(config);
    const platforms = await (0, common_1.selectPlatforms)(config, selectedPlatformName);
    await Promise.all(platforms.map(platformName => {
        return doctor(config, platformName);
    }));
}
exports.doctorCommand = doctorCommand;
async function doctorCore(config) {
    const [cliVersion, coreVersion, androidVersion, iosVersion] = await Promise.all([
        (0, subprocess_1.getCommandOutput)('npm', ['info', '@capacitor/cli', 'version']),
        (0, subprocess_1.getCommandOutput)('npm', ['info', '@capacitor/core', 'version']),
        (0, subprocess_1.getCommandOutput)('npm', ['info', '@capacitor/android', 'version']),
        (0, subprocess_1.getCommandOutput)('npm', ['info', '@capacitor/ios', 'version']),
    ]);
    log_1.output.write(`${colors_1.default.strong('Latest Dependencies:')}\n\n` +
        `  @capacitor/cli: ${colors_1.default.weak(cliVersion !== null && cliVersion !== void 0 ? cliVersion : 'unknown')}\n` +
        `  @capacitor/core: ${colors_1.default.weak(coreVersion !== null && coreVersion !== void 0 ? coreVersion : 'unknown')}\n` +
        `  @capacitor/android: ${colors_1.default.weak(androidVersion !== null && androidVersion !== void 0 ? androidVersion : 'unknown')}\n` +
        `  @capacitor/ios: ${colors_1.default.weak(iosVersion !== null && iosVersion !== void 0 ? iosVersion : 'unknown')}\n\n` +
        `${colors_1.default.strong('Installed Dependencies:')}\n\n`);
    await printInstalledPackages(config);
    log_1.output.write('\n');
}
exports.doctorCore = doctorCore;
async function printInstalledPackages(config) {
    const packageNames = [
        '@capacitor/cli',
        '@capacitor/core',
        '@capacitor/android',
        '@capacitor/ios',
    ];
    await Promise.all(packageNames.map(async (packageName) => {
        const packagePath = (0, node_1.resolveNode)(config.app.rootDir, packageName, 'package.json');
        await printPackageVersion(packageName, packagePath);
    }));
}
async function printPackageVersion(packageName, packagePath) {
    let version;
    if (packagePath) {
        version = (await (0, utils_fs_1.readJSON)(packagePath)).version;
    }
    log_1.output.write(`  ${packageName}: ${colors_1.default.weak(version || 'not installed')}\n`);
}
async function doctor(config, platformName) {
    if (platformName === config.ios.name) {
        await (0, doctor_2.doctorIOS)(config);
    }
    else if (platformName === config.android.name) {
        await (0, doctor_1.doctorAndroid)(config);
    }
    else if (platformName === config.web.name) {
        return Promise.resolve();
    }
    else {
        throw `Platform ${platformName} is not valid.`;
    }
}
exports.doctor = doctor;
