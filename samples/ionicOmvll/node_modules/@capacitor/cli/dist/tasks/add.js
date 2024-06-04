"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.addCommand = void 0;
const tslib_1 = require("tslib");
const utils_fs_1 = require("@ionic/utils-fs");
const utils_terminal_1 = require("@ionic/utils-terminal");
const add_1 = require("../android/add");
const common_1 = require("../android/common");
const colors_1 = tslib_1.__importDefault(require("../colors"));
const common_2 = require("../common");
const errors_1 = require("../errors");
const add_2 = require("../ios/add");
const common_3 = require("../ios/common");
const log_1 = require("../log");
const sync_1 = require("./sync");
async function addCommand(config, selectedPlatformName) {
    var _a;
    if (selectedPlatformName && !(await (0, common_2.isValidPlatform)(selectedPlatformName))) {
        const platformDir = (0, common_2.resolvePlatform)(config, selectedPlatformName);
        if (platformDir) {
            await (0, common_2.runPlatformHook)(config, selectedPlatformName, platformDir, 'capacitor:add');
        }
        else {
            let msg = `Platform ${colors_1.default.input(selectedPlatformName)} not found.`;
            if (await (0, common_2.isValidCommunityPlatform)(selectedPlatformName)) {
                msg += `\nTry installing ${colors_1.default.strong(`@capacitor-community/${selectedPlatformName}`)} and adding the platform again.`;
            }
            if (await (0, common_2.isValidEnterprisePlatform)(selectedPlatformName)) {
                msg +=
                    `\nThis is an enterprise platform and @ionic-enterprise/capacitor-${selectedPlatformName} is not installed.\n` +
                        `To learn how to use this platform, visit https://ionic.io/docs/${selectedPlatformName}`;
            }
            log_1.logger.error(msg);
        }
    }
    else {
        const knownPlatforms = await (0, common_2.getKnownPlatforms)();
        const platformName = await (0, common_2.promptForPlatform)(knownPlatforms, `Please choose a platform to add:`, selectedPlatformName);
        if (platformName === config.web.name) {
            webWarning();
            return;
        }
        const existingPlatformDir = await (0, common_2.getProjectPlatformDirectory)(config, platformName);
        if (existingPlatformDir) {
            (0, errors_1.fatal)(`${colors_1.default.input(platformName)} platform already exists.\n` +
                `To re-add this platform, first remove ${colors_1.default.strong((0, utils_terminal_1.prettyPath)(existingPlatformDir))}, then run this command again.\n` +
                `${colors_1.default.strong('WARNING')}: Your native project will be completely removed.`);
        }
        try {
            await (0, common_2.check)([
                () => (0, common_2.checkPackage)(),
                () => (0, common_2.checkAppConfig)(config),
                ...addChecks(config, platformName),
            ]);
            await doAdd(config, platformName);
            await editPlatforms(config, platformName);
            if (await (0, utils_fs_1.pathExists)(config.app.webDirAbs)) {
                await (0, sync_1.sync)(config, platformName, false, false);
                if (platformName === config.android.name) {
                    await (0, common_2.runTask)('Syncing Gradle', async () => {
                        return (0, add_1.createLocalProperties)(config.android.platformDirAbs);
                    });
                }
            }
            else {
                log_1.logger.warn(`${colors_1.default.success(colors_1.default.strong('sync'))} could not run--missing ${colors_1.default.strong(config.app.webDir)} directory.`);
            }
            printNextSteps(platformName);
        }
        catch (e) {
            if (!(0, errors_1.isFatal)(e)) {
                (0, errors_1.fatal)((_a = e.stack) !== null && _a !== void 0 ? _a : e);
            }
            throw e;
        }
    }
}
exports.addCommand = addCommand;
function printNextSteps(platformName) {
    (0, log_1.logSuccess)(`${colors_1.default.strong(platformName)} platform added!`);
    log_1.output.write(`Follow the Developer Workflow guide to get building:\n${colors_1.default.strong(`https://capacitorjs.com/docs/basics/workflow`)}\n`);
}
function addChecks(config, platformName) {
    if (platformName === config.ios.name) {
        return [
            () => (0, common_3.checkIOSPackage)(config),
            () => (0, common_3.checkBundler)(config) || (0, common_3.checkCocoaPods)(config),
        ];
    }
    else if (platformName === config.android.name) {
        return [() => (0, common_1.checkAndroidPackage)(config)];
    }
    else if (platformName === config.web.name) {
        return [];
    }
    else {
        throw `Platform ${platformName} is not valid.`;
    }
}
async function doAdd(config, platformName) {
    await (0, common_2.runTask)(colors_1.default.success(colors_1.default.strong('add')), async () => {
        if (platformName === config.ios.name) {
            await (0, add_2.addIOS)(config);
        }
        else if (platformName === config.android.name) {
            await (0, add_1.addAndroid)(config);
        }
    });
}
async function editPlatforms(config, platformName) {
    if (platformName === config.ios.name) {
        await (0, common_3.editProjectSettingsIOS)(config);
    }
    else if (platformName === config.android.name) {
        await (0, common_1.editProjectSettingsAndroid)(config);
    }
}
function webWarning() {
    log_1.logger.error(`Not adding platform ${colors_1.default.strong('web')}.\n` +
        `In Capacitor, the web platform is just your web app! For example, if you have a React or Angular project, the web platform is that project.\n` +
        `To add Capacitor functionality to your web app, follow the Web Getting Started Guide: ${colors_1.default.strong('https://capacitorjs.com/docs/web')}`);
}
