"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.open = exports.openCommand = void 0;
const tslib_1 = require("tslib");
const open_1 = require("../android/open");
const colors_1 = tslib_1.__importDefault(require("../colors"));
const common_1 = require("../common");
const errors_1 = require("../errors");
const open_2 = require("../ios/open");
const log_1 = require("../log");
async function openCommand(config, selectedPlatformName) {
    var _a;
    if (selectedPlatformName && !(await (0, common_1.isValidPlatform)(selectedPlatformName))) {
        const platformDir = (0, common_1.resolvePlatform)(config, selectedPlatformName);
        if (platformDir) {
            await (0, common_1.runPlatformHook)(config, selectedPlatformName, platformDir, 'capacitor:open');
        }
        else {
            log_1.logger.error(`Platform ${colors_1.default.input(selectedPlatformName)} not found.`);
        }
    }
    else {
        const platforms = await (0, common_1.selectPlatforms)(config, selectedPlatformName);
        let platformName;
        if (platforms.length === 1) {
            platformName = platforms[0];
        }
        else {
            platformName = await (0, common_1.promptForPlatform)(platforms.filter(createOpenablePlatformFilter(config)), `Please choose a platform to open:`);
        }
        try {
            await open(config, platformName);
        }
        catch (e) {
            if (!(0, errors_1.isFatal)(e)) {
                (0, errors_1.fatal)((_a = e.stack) !== null && _a !== void 0 ? _a : e);
            }
            throw e;
        }
    }
}
exports.openCommand = openCommand;
function createOpenablePlatformFilter(config) {
    return platform => platform === config.ios.name || platform === config.android.name;
}
async function open(config, platformName) {
    if (platformName === config.ios.name) {
        await (0, common_1.runTask)('Opening the Xcode workspace...', () => {
            return (0, open_2.openIOS)(config);
        });
    }
    else if (platformName === config.android.name) {
        return (0, open_1.openAndroid)(config);
    }
    else if (platformName === config.web.name) {
        return Promise.resolve();
    }
    else {
        throw `Platform ${platformName} is not valid.`;
    }
}
exports.open = open;
