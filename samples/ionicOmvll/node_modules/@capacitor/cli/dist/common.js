"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.parseApkNameFromFlavor = exports.checkJDKMajorVersion = exports.resolvePlatform = exports.checkPlatformVersions = exports.getAddedPlatforms = exports.getPlatformTargetName = exports.promptForPlatformTarget = exports.promptForPlatform = exports.isValidEnterprisePlatform = exports.getKnownEnterprisePlatforms = exports.isValidCommunityPlatform = exports.getKnownCommunityPlatforms = exports.isValidPlatform = exports.getKnownPlatforms = exports.selectPlatforms = exports.getProjectPlatformDirectory = exports.getCLIVersion = exports.getCoreVersion = exports.getCapacitorPackageVersion = exports.requireCapacitorPackage = exports.getCapacitorPackage = exports.runTask = exports.runPlatformHook = exports.wait = exports.checkAppName = exports.checkAppId = exports.checkAppDir = exports.checkAppConfig = exports.checkCapacitorPlatform = exports.checkPackage = exports.checkWebDir = exports.check = void 0;
const tslib_1 = require("tslib");
const utils_fs_1 = require("@ionic/utils-fs");
const utils_terminal_1 = require("@ionic/utils-terminal");
const path_1 = require("path");
const colors_1 = tslib_1.__importDefault(require("./colors"));
const errors_1 = require("./errors");
const log_1 = require("./log");
const monorepotools_1 = require("./util/monorepotools");
const node_1 = require("./util/node");
const subprocess_1 = require("./util/subprocess");
async function check(checks) {
    const results = await Promise.all(checks.map(f => f()));
    const errors = results.filter(r => r != null);
    if (errors.length > 0) {
        throw errors.join('\n');
    }
}
exports.check = check;
async function checkWebDir(config) {
    var _a;
    // We can skip checking the web dir if a server URL is set.
    if ((_a = config.app.extConfig.server) === null || _a === void 0 ? void 0 : _a.url) {
        return null;
    }
    const invalidFolders = ['', '.', '..', '../', './'];
    if (invalidFolders.includes(config.app.webDir)) {
        return `"${config.app.webDir}" is not a valid value for webDir`;
    }
    if (!(await (0, utils_fs_1.pathExists)(config.app.webDirAbs))) {
        return (`Could not find the web assets directory: ${colors_1.default.strong((0, utils_terminal_1.prettyPath)(config.app.webDirAbs))}.\n` +
            `Please create it and make sure it has an ${colors_1.default.strong('index.html')} file. You can change the path of this directory in ${colors_1.default.strong(config.app.extConfigName)} (${colors_1.default.input('webDir')} option). You may need to compile the web assets for your app (typically ${colors_1.default.input('npm run build')}). More info: ${colors_1.default.strong('https://capacitorjs.com/docs/basics/workflow#sync-your-project')}`);
    }
    if (!(await (0, utils_fs_1.pathExists)((0, path_1.join)(config.app.webDirAbs, 'index.html')))) {
        return (`The web assets directory (${colors_1.default.strong((0, utils_terminal_1.prettyPath)(config.app.webDirAbs))}) must contain an ${colors_1.default.strong('index.html')} file.\n` +
            `It will be the entry point for the web portion of the Capacitor app.`);
    }
    return null;
}
exports.checkWebDir = checkWebDir;
async function checkPackage() {
    if (!(await (0, utils_fs_1.pathExists)('package.json'))) {
        if (await (0, utils_fs_1.pathExists)('project.json')) {
            return null;
        }
        else {
            return (`The Capacitor CLI needs to run at the root of an npm package or in a valid NX monorepo.\n` +
                `Make sure you have a package.json or project.json file in the directory where you run the Capacitor CLI.\n` +
                `More info: ${colors_1.default.strong('https://docs.npmjs.com/cli/init')}`);
        }
    }
    return null;
}
exports.checkPackage = checkPackage;
async function checkCapacitorPlatform(config, platform) {
    const pkg = await getCapacitorPackage(config, platform);
    if (!pkg) {
        return (`Could not find the ${colors_1.default.input(platform)} platform.\n` +
            `You must install it in your project first, e.g. w/ ${colors_1.default.input(`npm install @capacitor/${platform}`)}`);
    }
    return null;
}
exports.checkCapacitorPlatform = checkCapacitorPlatform;
async function checkAppConfig(config) {
    if (!config.app.appId) {
        return (`Missing ${colors_1.default.input('appId')} for new platform.\n` +
            `Please add it in ${config.app.extConfigName} or run ${colors_1.default.input('npx cap init')}.`);
    }
    if (!config.app.appName) {
        return (`Missing ${colors_1.default.input('appName')} for new platform.\n` +
            `Please add it in ${config.app.extConfigName} or run ${colors_1.default.input('npx cap init')}.`);
    }
    const appIdError = await checkAppId(config, config.app.appId);
    if (appIdError) {
        return appIdError;
    }
    const appNameError = await checkAppName(config, config.app.appName);
    if (appNameError) {
        return appNameError;
    }
    return null;
}
exports.checkAppConfig = checkAppConfig;
async function checkAppDir(config, dir) {
    if (!/^\S*$/.test(dir)) {
        return `Your app directory should not contain spaces`;
    }
    return null;
}
exports.checkAppDir = checkAppDir;
async function checkAppId(config, id) {
    if (!id) {
        return `Invalid App ID. Must be in Java package form with no dashes (ex: com.example.app)`;
    }
    if (/^[a-z][a-z0-9_]*(\.[a-z0-9_]+)+$/.test(id.toLowerCase())) {
        return null;
    }
    return `Invalid App ID "${id}". Must be in Java package form with no dashes (ex: com.example.app)`;
}
exports.checkAppId = checkAppId;
async function checkAppName(config, name) {
    // We allow pretty much anything right now, have fun
    if (!(name === null || name === void 0 ? void 0 : name.length)) {
        return `Must provide an app name. For example: 'Spacebook'`;
    }
    return null;
}
exports.checkAppName = checkAppName;
async function wait(time) {
    return new Promise(resolve => setTimeout(resolve, time));
}
exports.wait = wait;
async function runPlatformHook(config, platformName, platformDir, hook) {
    var _a;
    const { spawn } = await Promise.resolve().then(() => tslib_1.__importStar(require('child_process')));
    let pkg;
    if ((0, monorepotools_1.isNXMonorepo)(platformDir)) {
        pkg = await (0, utils_fs_1.readJSON)((0, path_1.join)((0, monorepotools_1.findNXMonorepoRoot)(platformDir), 'package.json'));
    }
    else {
        pkg = await (0, utils_fs_1.readJSON)((0, path_1.join)(platformDir, 'package.json'));
    }
    const cmd = (_a = pkg.scripts) === null || _a === void 0 ? void 0 : _a[hook];
    if (!cmd) {
        return;
    }
    return new Promise((resolve, reject) => {
        const p = spawn(cmd, {
            stdio: 'inherit',
            shell: true,
            cwd: platformDir,
            env: {
                INIT_CWD: platformDir,
                CAPACITOR_ROOT_DIR: config.app.rootDir,
                CAPACITOR_WEB_DIR: config.app.webDirAbs,
                CAPACITOR_CONFIG: JSON.stringify(config.app.extConfig),
                CAPACITOR_PLATFORM_NAME: platformName,
                ...process.env,
            },
        });
        p.on('close', () => {
            resolve();
        });
        p.on('error', err => {
            reject(err);
        });
    });
}
exports.runPlatformHook = runPlatformHook;
async function runTask(title, fn) {
    const chain = log_1.output.createTaskChain();
    chain.next(title);
    try {
        const value = await fn();
        chain.end();
        return value;
    }
    catch (e) {
        chain.fail();
        throw e;
    }
}
exports.runTask = runTask;
async function getCapacitorPackage(config, name) {
    const packagePath = (0, node_1.resolveNode)(config.app.rootDir, `@capacitor/${name}`, 'package.json');
    if (!packagePath) {
        return null;
    }
    return (0, utils_fs_1.readJSON)(packagePath);
}
exports.getCapacitorPackage = getCapacitorPackage;
async function requireCapacitorPackage(config, name) {
    const pkg = await getCapacitorPackage(config, name);
    if (!pkg) {
        (0, errors_1.fatal)(`Unable to find node_modules/@capacitor/${name}.\n` +
            `Are you sure ${colors_1.default.strong(`@capacitor/${name}`)} is installed?`);
    }
    return pkg;
}
exports.requireCapacitorPackage = requireCapacitorPackage;
async function getCapacitorPackageVersion(config, platform) {
    return (await requireCapacitorPackage(config, platform)).version;
}
exports.getCapacitorPackageVersion = getCapacitorPackageVersion;
async function getCoreVersion(config) {
    return getCapacitorPackageVersion(config, 'core');
}
exports.getCoreVersion = getCoreVersion;
async function getCLIVersion(config) {
    return getCapacitorPackageVersion(config, 'cli');
}
exports.getCLIVersion = getCLIVersion;
function getPlatformDirectory(config, platform) {
    switch (platform) {
        case 'android':
            return config.android.platformDirAbs;
        case 'ios':
            return config.ios.platformDirAbs;
        case 'web':
            return config.web.platformDirAbs;
    }
    return null;
}
async function getProjectPlatformDirectory(config, platform) {
    const platformPath = getPlatformDirectory(config, platform);
    if (platformPath && (await (0, utils_fs_1.pathExists)(platformPath))) {
        return platformPath;
    }
    return null;
}
exports.getProjectPlatformDirectory = getProjectPlatformDirectory;
async function selectPlatforms(config, selectedPlatformName) {
    if (selectedPlatformName) {
        // already passed in a platform name
        const platformName = selectedPlatformName.toLowerCase().trim();
        if (!(await isValidPlatform(platformName))) {
            (0, errors_1.fatal)(`Invalid platform: ${colors_1.default.input(platformName)}`);
        }
        else if (!(await getProjectPlatformDirectory(config, platformName))) {
            if (platformName === 'web') {
                (0, errors_1.fatal)(`Could not find the web platform directory.\n` +
                    `Make sure ${colors_1.default.strong(config.app.webDir)} exists.`);
            }
            (0, errors_1.fatal)(`${colors_1.default.strong(platformName)} platform has not been added yet.\n` +
                `See the docs for adding the ${colors_1.default.strong(platformName)} platform: ${colors_1.default.strong(`https://capacitorjs.com/docs/${platformName}#adding-the-${platformName}-platform`)}`);
        }
        // return the platform in an string array
        return [platformName];
    }
    // wasn't given a platform name, so let's
    // get the platforms that have already been created
    return getAddedPlatforms(config);
}
exports.selectPlatforms = selectPlatforms;
async function getKnownPlatforms() {
    return ['web', 'android', 'ios'];
}
exports.getKnownPlatforms = getKnownPlatforms;
async function isValidPlatform(platform) {
    return (await getKnownPlatforms()).includes(platform);
}
exports.isValidPlatform = isValidPlatform;
async function getKnownCommunityPlatforms() {
    return ['electron'];
}
exports.getKnownCommunityPlatforms = getKnownCommunityPlatforms;
async function isValidCommunityPlatform(platform) {
    return (await getKnownCommunityPlatforms()).includes(platform);
}
exports.isValidCommunityPlatform = isValidCommunityPlatform;
async function getKnownEnterprisePlatforms() {
    return ['windows'];
}
exports.getKnownEnterprisePlatforms = getKnownEnterprisePlatforms;
async function isValidEnterprisePlatform(platform) {
    return (await getKnownEnterprisePlatforms()).includes(platform);
}
exports.isValidEnterprisePlatform = isValidEnterprisePlatform;
async function promptForPlatform(platforms, promptMessage, selectedPlatformName) {
    const { prompt } = await Promise.resolve().then(() => tslib_1.__importStar(require('prompts')));
    if (!selectedPlatformName) {
        const answers = await prompt([
            {
                type: 'select',
                name: 'mode',
                message: promptMessage,
                choices: platforms.map(p => ({ title: p, value: p })),
            },
        ], { onCancel: () => process.exit(1) });
        return answers.mode.toLowerCase().trim();
    }
    const platformName = selectedPlatformName.toLowerCase().trim();
    if (!(await isValidPlatform(platformName))) {
        const knownPlatforms = await getKnownPlatforms();
        (0, errors_1.fatal)(`Invalid platform: ${colors_1.default.input(platformName)}.\n` +
            `Valid platforms include: ${knownPlatforms.join(', ')}`);
    }
    return platformName;
}
exports.promptForPlatform = promptForPlatform;
async function promptForPlatformTarget(targets, selectedTarget) {
    const { prompt } = await Promise.resolve().then(() => tslib_1.__importStar(require('prompts')));
    const validTargets = targets.filter(t => t.id !== undefined);
    if (!selectedTarget) {
        if (validTargets.length === 1) {
            return validTargets[0];
        }
        else {
            const answers = await prompt([
                {
                    type: 'select',
                    name: 'target',
                    message: 'Please choose a target device:',
                    choices: validTargets.map(t => ({
                        title: `${getPlatformTargetName(t)} (${t.id})`,
                        value: t,
                    })),
                },
            ], { onCancel: () => process.exit(1) });
            return answers.target;
        }
    }
    const targetID = selectedTarget.trim();
    const target = targets.find(t => t.id === targetID);
    if (!target) {
        (0, errors_1.fatal)(`Invalid target ID: ${colors_1.default.input(targetID)}.\n` +
            `Valid targets are: ${targets.map(t => t.id).join(', ')}`);
    }
    return target;
}
exports.promptForPlatformTarget = promptForPlatformTarget;
function getPlatformTargetName(target) {
    var _a, _b, _c;
    return `${(_c = (_b = (_a = target.name) !== null && _a !== void 0 ? _a : target.model) !== null && _b !== void 0 ? _b : target.id) !== null && _c !== void 0 ? _c : '?'}${target.virtual
        ? ` (${target.platform === 'ios' ? 'simulator' : 'emulator'})`
        : ''}`;
}
exports.getPlatformTargetName = getPlatformTargetName;
async function getAddedPlatforms(config) {
    const platforms = [];
    if (await getProjectPlatformDirectory(config, config.android.name)) {
        platforms.push(config.android.name);
    }
    if (await getProjectPlatformDirectory(config, config.ios.name)) {
        platforms.push(config.ios.name);
    }
    platforms.push(config.web.name);
    return platforms;
}
exports.getAddedPlatforms = getAddedPlatforms;
async function checkPlatformVersions(config, platform) {
    const semver = await Promise.resolve().then(() => tslib_1.__importStar(require('semver')));
    const coreVersion = await getCoreVersion(config);
    const platformVersion = await getCapacitorPackageVersion(config, platform);
    if (semver.diff(coreVersion, platformVersion) === 'minor' ||
        semver.diff(coreVersion, platformVersion) === 'major') {
        log_1.logger.warn(`${colors_1.default.strong('@capacitor/core')}${colors_1.default.weak(`@${coreVersion}`)} version doesn't match ${colors_1.default.strong(`@capacitor/${platform}`)}${colors_1.default.weak(`@${platformVersion}`)} version.\n` +
            `Consider updating to a matching version, e.g. w/ ${colors_1.default.input(`npm install @capacitor/core@${platformVersion}`)}`);
    }
}
exports.checkPlatformVersions = checkPlatformVersions;
function resolvePlatform(config, platform) {
    if (platform[0] !== '@') {
        const core = (0, node_1.resolveNode)(config.app.rootDir, `@capacitor/${platform}`, 'package.json');
        if (core) {
            return (0, path_1.dirname)(core);
        }
        const community = (0, node_1.resolveNode)(config.app.rootDir, `@capacitor-community/${platform}`, 'package.json');
        if (community) {
            return (0, path_1.dirname)(community);
        }
        const enterprise = (0, node_1.resolveNode)(config.app.rootDir, `@ionic-enterprise/capacitor-${platform}`, 'package.json');
        if (enterprise) {
            return (0, path_1.dirname)(enterprise);
        }
    }
    // third-party
    const thirdParty = (0, node_1.resolveNode)(config.app.rootDir, platform, 'package.json');
    if (thirdParty) {
        return (0, path_1.dirname)(thirdParty);
    }
    return null;
}
exports.resolvePlatform = resolvePlatform;
async function checkJDKMajorVersion() {
    try {
        const string = await (0, subprocess_1.runCommand)('java', ['--version']);
        const versionRegex = RegExp(/([0-9]+)\.?([0-9]*)\.?([0-9]*)/);
        const versionMatch = versionRegex.exec(string);
        if (versionMatch === null) {
            return -1;
        }
        const firstVersionNumber = parseInt(versionMatch[1]);
        const secondVersionNumber = parseInt(versionMatch[2]);
        if (typeof firstVersionNumber === 'number' && firstVersionNumber != 1) {
            return firstVersionNumber;
        }
        else if (typeof secondVersionNumber === 'number' &&
            firstVersionNumber == 1 &&
            secondVersionNumber < 9) {
            return secondVersionNumber;
        }
        else {
            return -1;
        }
    }
    catch (e) {
        return -1;
    }
}
exports.checkJDKMajorVersion = checkJDKMajorVersion;
function parseApkNameFromFlavor(flavor) {
    const convertedName = flavor.replace(/([A-Z])/g, '$1').toLowerCase();
    return `app-${convertedName ? `${convertedName}-` : ''}debug.apk`;
}
exports.parseApkNameFromFlavor = parseApkNameFromFlavor;
