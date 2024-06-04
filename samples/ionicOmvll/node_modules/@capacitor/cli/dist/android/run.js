"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.runAndroid = void 0;
const tslib_1 = require("tslib");
const debug_1 = tslib_1.__importDefault(require("debug"));
const path_1 = require("path");
const colors_1 = tslib_1.__importDefault(require("../colors"));
const common_1 = require("../common");
const native_run_1 = require("../util/native-run");
const subprocess_1 = require("../util/subprocess");
const debug = (0, debug_1.default)('capacitor:android:run');
async function runAndroid(config, { target: selectedTarget, flavor: selectedFlavor, forwardPorts: selectedPorts, }) {
    var _a;
    const target = await (0, common_1.promptForPlatformTarget)(await (0, native_run_1.getPlatformTargets)('android'), selectedTarget);
    const runFlavor = selectedFlavor || ((_a = config.android) === null || _a === void 0 ? void 0 : _a.flavor) || '';
    const arg = `assemble${runFlavor}Debug`;
    const gradleArgs = [arg];
    debug('Invoking ./gradlew with args: %O', gradleArgs);
    try {
        await (0, common_1.runTask)('Running Gradle build', async () => (0, subprocess_1.runCommand)('./gradlew', gradleArgs, {
            cwd: config.android.platformDirAbs,
        }));
    }
    catch (e) {
        if (e.includes('EACCES')) {
            throw `gradlew file does not have executable permissions. This can happen if the Android platform was added on a Windows machine. Please run ${colors_1.default.strong(`chmod +x ./${config.android.platformDir}/gradlew`)} and try again.`;
        }
        else {
            throw e;
        }
    }
    const pathToApk = `${config.android.platformDirAbs}/${config.android.appDir}/build/outputs/apk${runFlavor !== '' ? '/' + runFlavor : ''}/debug`;
    const apkName = (0, common_1.parseApkNameFromFlavor)(runFlavor);
    const apkPath = (0, path_1.resolve)(pathToApk, apkName);
    const nativeRunArgs = ['android', '--app', apkPath, '--target', target.id];
    if (selectedPorts) {
        nativeRunArgs.push('--forward', `${selectedPorts}`);
    }
    debug('Invoking native-run with args: %O', nativeRunArgs);
    await (0, common_1.runTask)(`Deploying ${colors_1.default.strong(apkName)} to ${colors_1.default.input(target.id)}`, async () => (0, native_run_1.runNativeRun)(nativeRunArgs));
}
exports.runAndroid = runAndroid;
