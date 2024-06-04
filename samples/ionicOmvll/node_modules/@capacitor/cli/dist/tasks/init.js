"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.initCommand = void 0;
const tslib_1 = require("tslib");
const open_1 = tslib_1.__importDefault(require("open"));
const path_1 = require("path");
const colors_1 = tslib_1.__importDefault(require("../colors"));
const common_1 = require("../common");
const config_1 = require("../config");
const cordova_1 = require("../cordova");
const errors_1 = require("../errors");
const framework_configs_1 = require("../framework-configs");
const log_1 = require("../log");
const sysconfig_1 = require("../sysconfig");
const node_1 = require("../util/node");
const term_1 = require("../util/term");
async function initCommand(config, name, id, webDirFromCLI) {
    var _a, _b;
    try {
        if (!(0, term_1.checkInteractive)(name, id)) {
            return;
        }
        if (config.app.extConfigType !== 'json') {
            (0, errors_1.fatal)(`Cannot run ${colors_1.default.input('init')} for a project using a non-JSON configuration file.\n` +
                `Delete ${colors_1.default.strong(config.app.extConfigName)} and try again.`);
        }
        const isNewConfig = Object.keys(config.app.extConfig).length === 0;
        const tsInstalled = !!(0, node_1.resolveNode)(config.app.rootDir, 'typescript');
        const appName = await getName(config, name);
        const appId = await getAppId(config, id);
        const webDir = (0, term_1.isInteractive)()
            ? await getWebDir(config, webDirFromCLI)
            : (_a = webDirFromCLI !== null && webDirFromCLI !== void 0 ? webDirFromCLI : config.app.extConfig.webDir) !== null && _a !== void 0 ? _a : 'www';
        await (0, common_1.check)([
            () => (0, common_1.checkAppName)(config, appName),
            () => (0, common_1.checkAppId)(config, appId),
        ]);
        const cordova = await (0, cordova_1.getCordovaPreferences)(config);
        await runMergeConfig(config, {
            appId,
            appName,
            webDir,
            cordova,
        }, isNewConfig && tsInstalled ? 'ts' : 'json');
    }
    catch (e) {
        if (!(0, errors_1.isFatal)(e)) {
            log_1.output.write('Usage: npx cap init appName appId\n' +
                'Example: npx cap init "My App" "com.example.myapp"\n\n');
            (0, errors_1.fatal)((_b = e.stack) !== null && _b !== void 0 ? _b : e);
        }
        throw e;
    }
}
exports.initCommand = initCommand;
async function getName(config, name) {
    var _a;
    if (!name) {
        const answers = await (0, log_1.logPrompt)(`${colors_1.default.strong(`What is the name of your app?`)}\n` +
            `This should be a human-friendly app name, like what you'd see in the App Store.`, {
            type: 'text',
            name: 'name',
            message: `Name`,
            initial: config.app.appName
                ? config.app.appName
                : (_a = config.app.package.name) !== null && _a !== void 0 ? _a : 'App',
        });
        return answers.name;
    }
    return name;
}
async function getAppId(config, id) {
    if (!id) {
        const answers = await (0, log_1.logPrompt)(`${colors_1.default.strong(`What should be the Package ID for your app?`)}\n` +
            `Package IDs (aka Bundle ID in iOS and Application ID in Android) are unique identifiers for apps. They must be in reverse domain name notation, generally representing a domain name that you or your company owns.`, {
            type: 'text',
            name: 'id',
            message: `Package ID`,
            initial: config.app.appId ? config.app.appId : 'com.example.app',
        });
        return answers.id;
    }
    return id;
}
async function getWebDir(config, webDir) {
    if (!webDir) {
        const framework = (0, framework_configs_1.detectFramework)(config);
        if (framework === null || framework === void 0 ? void 0 : framework.webDir) {
            return framework.webDir;
        }
        const answers = await (0, log_1.logPrompt)(`${colors_1.default.strong(`What is the web asset directory for your app?`)}\n` +
            `This directory should contain the final ${colors_1.default.strong('index.html')} of your app.`, {
            type: 'text',
            name: 'webDir',
            message: `Web asset directory`,
            initial: config.app.webDir ? config.app.webDir : 'www',
        });
        return answers.webDir;
    }
    return webDir;
}
async function runMergeConfig(config, extConfig, type) {
    const configDirectory = (0, path_1.dirname)(config.app.extConfigFilePath);
    const newConfigPath = (0, path_1.resolve)(configDirectory, type === 'ts' ? config_1.CONFIG_FILE_NAME_TS : config_1.CONFIG_FILE_NAME_JSON);
    await (0, common_1.runTask)(`Creating ${colors_1.default.strong((0, path_1.basename)(newConfigPath))} in ${colors_1.default.input(config.app.rootDir)}`, async () => {
        await mergeConfig(config, extConfig, newConfigPath);
    });
    printNextSteps((0, path_1.basename)(newConfigPath));
    if ((0, term_1.isInteractive)()) {
        let sysconfig = await (0, sysconfig_1.readConfig)();
        if (typeof sysconfig.signup === 'undefined') {
            const signup = await promptToSignup();
            sysconfig = { ...sysconfig, signup };
            await (0, sysconfig_1.writeConfig)(sysconfig);
        }
    }
}
async function mergeConfig(config, extConfig, newConfigPath) {
    const oldConfig = { ...config.app.extConfig };
    const newConfig = { ...oldConfig, ...extConfig };
    await (0, config_1.writeConfig)(newConfig, newConfigPath);
}
function printNextSteps(newConfigName) {
    (0, log_1.logSuccess)(`${colors_1.default.strong(newConfigName)} created!`);
    log_1.output.write(`\nNext steps: \n${colors_1.default.strong(`https://capacitorjs.com/docs/getting-started#where-to-go-next`)}\n`);
}
async function promptToSignup() {
    const answers = await (0, log_1.logPrompt)(`Join the Ionic Community! 💙\n` +
        `Connect with millions of developers on the Ionic Forum and get access to live events, news updates, and more.`, {
        type: 'confirm',
        name: 'create',
        message: `Create free Ionic account?`,
        initial: true,
    });
    if (answers.create) {
        (0, open_1.default)(`http://ionicframework.com/signup?source=capacitor`);
    }
    return answers.create;
}
