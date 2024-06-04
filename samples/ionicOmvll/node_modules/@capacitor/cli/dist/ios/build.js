"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.buildiOS = void 0;
const tslib_1 = require("tslib");
const utils_fs_1 = require("@ionic/utils-fs");
const path_1 = require("path");
const rimraf_1 = tslib_1.__importDefault(require("rimraf"));
const common_1 = require("../common");
const log_1 = require("../log");
const spm_1 = require("../util/spm");
const subprocess_1 = require("../util/subprocess");
async function buildiOS(config, buildOptions) {
    var _a;
    const theScheme = (_a = buildOptions.scheme) !== null && _a !== void 0 ? _a : 'App';
    const packageManager = await (0, spm_1.checkPackageManager)(config);
    let typeOfBuild;
    let projectName;
    if (packageManager == 'Cocoapods') {
        typeOfBuild = '-workspace';
        projectName = (0, path_1.basename)(await config.ios.nativeXcodeWorkspaceDirAbs);
    }
    else {
        typeOfBuild = '-project';
        projectName = (0, path_1.basename)(await config.ios.nativeXcodeProjDirAbs);
    }
    await (0, common_1.runTask)('Building xArchive', async () => (0, subprocess_1.runCommand)('xcodebuild', [
        typeOfBuild,
        projectName,
        '-scheme',
        `${theScheme}`,
        '-destination',
        `generic/platform=iOS`,
        '-archivePath',
        `${theScheme}.xcarchive`,
        'archive',
    ], {
        cwd: config.ios.nativeProjectDirAbs,
    }));
    const archivePlistContents = `<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
<key>method</key>
<string>app-store</string>
</dict>
</plist>`;
    const archivePlistPath = (0, path_1.join)(`${config.ios.nativeProjectDirAbs}`, 'archive.plist');
    (0, utils_fs_1.writeFileSync)(archivePlistPath, archivePlistContents);
    await (0, common_1.runTask)('Building IPA', async () => (0, subprocess_1.runCommand)('xcodebuild', [
        'archive',
        '-archivePath',
        `${theScheme}.xcarchive`,
        '-exportArchive',
        '-exportOptionsPlist',
        'archive.plist',
        '-exportPath',
        'output',
        '-allowProvisioningUpdates',
        '-configuration',
        buildOptions.configuration,
    ], {
        cwd: config.ios.nativeProjectDirAbs,
    }));
    await (0, common_1.runTask)('Cleaning up', async () => {
        (0, utils_fs_1.unlinkSync)(archivePlistPath);
        rimraf_1.default.sync((0, path_1.join)(config.ios.nativeProjectDirAbs, `${theScheme}.xcarchive`));
    });
    (0, log_1.logSuccess)(`Successfully generated an IPA at: ${(0, path_1.join)(config.ios.nativeProjectDirAbs, 'output')}`);
}
exports.buildiOS = buildiOS;
