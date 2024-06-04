"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.openAndroid = void 0;
const tslib_1 = require("tslib");
const utils_fs_1 = require("@ionic/utils-fs");
const debug_1 = tslib_1.__importDefault(require("debug"));
const open_1 = tslib_1.__importDefault(require("open"));
const colors_1 = tslib_1.__importDefault(require("../colors"));
const log_1 = require("../log");
const debug = (0, debug_1.default)('capacitor:android:open');
async function openAndroid(config) {
    const androidStudioPath = await config.android.studioPath;
    const dir = config.android.platformDirAbs;
    try {
        if (!(await (0, utils_fs_1.pathExists)(androidStudioPath))) {
            throw new Error(`Android Studio does not exist at: ${androidStudioPath}`);
        }
        await (0, open_1.default)(dir, { app: { name: androidStudioPath }, wait: false });
        log_1.logger.info(`Opening Android project at: ${colors_1.default.strong(config.android.platformDir)}.`);
    }
    catch (e) {
        debug('Error opening Android Studio: %O', e);
        log_1.logger.error('Unable to launch Android Studio. Is it installed?\n' +
            `Attempted to open Android Studio at: ${colors_1.default.strong(androidStudioPath)}\n` +
            `You can configure this with the ${colors_1.default.input('CAPACITOR_ANDROID_STUDIO_PATH')} environment variable.`);
    }
}
exports.openAndroid = openAndroid;
