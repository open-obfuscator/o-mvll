"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.doctorIOS = void 0;
const common_1 = require("../common");
const errors_1 = require("../errors");
const log_1 = require("../log");
const subprocess_1 = require("../util/subprocess");
const common_2 = require("./common");
async function doctorIOS(config) {
    var _a;
    // DOCTOR ideas for iOS:
    // plugin specific warnings
    // check cocoapods installed
    // check projects exist
    // check content in www === ios/www
    // check CLI versions
    // check plugins versions
    // check native project deps are up-to-date === npm install
    // check if npm install was updated
    // check online datebase of common errors
    // check if www folder is empty (index.html does not exist)
    try {
        await (0, common_1.check)([
            () => (0, common_2.checkBundler)(config) || (0, common_2.checkCocoaPods)(config),
            () => (0, common_1.checkWebDir)(config),
            checkXcode,
        ]);
        (0, log_1.logSuccess)('iOS looking great! 👌');
    }
    catch (e) {
        (0, errors_1.fatal)((_a = e.stack) !== null && _a !== void 0 ? _a : e);
    }
}
exports.doctorIOS = doctorIOS;
async function checkXcode() {
    if (!(await (0, subprocess_1.isInstalled)('xcodebuild'))) {
        return `Xcode is not installed`;
    }
    // const matches = output.match(/^Xcode (.*)/);
    // if (matches && matches.length === 2) {
    //   const minVersion = '9.0.0';
    //   const semver = await import('semver');
    //   console.log(matches[1]);
    //   if (semver.gt(minVersion, matches[1])) {
    //     return `Xcode version is too old, ${minVersion} is required`;
    //   }
    // }
    return null;
}
