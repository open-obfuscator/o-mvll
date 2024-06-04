"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.update = exports.updateChecks = exports.updateCommand = void 0;
const tslib_1 = require("tslib");
const update_1 = require("../android/update");
const colors_1 = tslib_1.__importDefault(require("../colors"));
const common_1 = require("../common");
const errors_1 = require("../errors");
const common_2 = require("../ios/common");
const update_2 = require("../ios/update");
const log_1 = require("../log");
const promise_1 = require("../util/promise");
async function updateCommand(config, selectedPlatformName, deployment) {
    var _a;
    if (selectedPlatformName && !(await (0, common_1.isValidPlatform)(selectedPlatformName))) {
        const platformDir = (0, common_1.resolvePlatform)(config, selectedPlatformName);
        if (platformDir) {
            await (0, common_1.runPlatformHook)(config, selectedPlatformName, platformDir, 'capacitor:update');
        }
        else {
            log_1.logger.error(`Platform ${colors_1.default.input(selectedPlatformName)} not found.`);
        }
    }
    else {
        const then = +new Date();
        const platforms = await (0, common_1.selectPlatforms)(config, selectedPlatformName);
        try {
            await (0, common_1.check)([() => (0, common_1.checkPackage)(), ...updateChecks(config, platforms)]);
            await (0, promise_1.allSerial)(platforms.map(platformName => async () => await update(config, platformName, deployment)));
            const now = +new Date();
            const diff = (now - then) / 1000;
            log_1.logger.info(`Update finished in ${diff}s`);
        }
        catch (e) {
            if (!(0, errors_1.isFatal)(e)) {
                (0, errors_1.fatal)((_a = e.stack) !== null && _a !== void 0 ? _a : e);
            }
            throw e;
        }
    }
}
exports.updateCommand = updateCommand;
function updateChecks(config, platforms) {
    const checks = [];
    for (const platformName of platforms) {
        if (platformName === config.ios.name) {
            checks.push(() => (0, common_2.checkBundler)(config) || (0, common_2.checkCocoaPods)(config));
        }
        else if (platformName === config.android.name) {
            continue;
        }
        else if (platformName === config.web.name) {
            continue;
        }
        else {
            throw `Platform ${platformName} is not valid.`;
        }
    }
    return checks;
}
exports.updateChecks = updateChecks;
async function update(config, platformName, deployment) {
    await (0, common_1.runTask)(colors_1.default.success(colors_1.default.strong(`update ${platformName}`)), async () => {
        await (0, common_1.runPlatformHook)(config, platformName, config.app.rootDir, 'capacitor:update:before');
        if (platformName === config.ios.name) {
            await (0, update_2.updateIOS)(config, deployment);
        }
        else if (platformName === config.android.name) {
            await (0, update_1.updateAndroid)(config);
        }
        await (0, common_1.runPlatformHook)(config, platformName, config.app.rootDir, 'capacitor:update:after');
    });
}
exports.update = update;
