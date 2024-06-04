"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.checkExternalConfig = exports.writeConfig = exports.loadConfig = exports.CONFIG_FILE_NAME_JSON = exports.CONFIG_FILE_NAME_JS = exports.CONFIG_FILE_NAME_TS = void 0;
const tslib_1 = require("tslib");
const utils_fs_1 = require("@ionic/utils-fs");
const debug_1 = tslib_1.__importDefault(require("debug"));
const path_1 = require("path");
const colors_1 = tslib_1.__importDefault(require("./colors"));
const common_1 = require("./common");
const errors_1 = require("./errors");
const log_1 = require("./log");
const fn_1 = require("./util/fn");
const js_1 = require("./util/js");
const monorepotools_1 = require("./util/monorepotools");
const node_1 = require("./util/node");
const promise_1 = require("./util/promise");
const subprocess_1 = require("./util/subprocess");
const debug = (0, debug_1.default)('capacitor:config');
exports.CONFIG_FILE_NAME_TS = 'capacitor.config.ts';
exports.CONFIG_FILE_NAME_JS = 'capacitor.config.js';
exports.CONFIG_FILE_NAME_JSON = 'capacitor.config.json';
async function loadConfig() {
    var _a, _b, _c, _d;
    const appRootDir = process.cwd();
    const cliRootDir = (0, path_1.dirname)(__dirname);
    const conf = await loadExtConfig(appRootDir);
    const depsForNx = await (async () => {
        var _a, _b;
        if ((0, monorepotools_1.isNXMonorepo)(appRootDir)) {
            const rootOfNXMonorepo = (0, monorepotools_1.findNXMonorepoRoot)(appRootDir);
            const pkgJSONOfMonorepoRoot = await (0, fn_1.tryFn)(utils_fs_1.readJSON, (0, path_1.resolve)(rootOfNXMonorepo, 'package.json'));
            const devDependencies = (_a = pkgJSONOfMonorepoRoot === null || pkgJSONOfMonorepoRoot === void 0 ? void 0 : pkgJSONOfMonorepoRoot.devDependencies) !== null && _a !== void 0 ? _a : {};
            const dependencies = (_b = pkgJSONOfMonorepoRoot === null || pkgJSONOfMonorepoRoot === void 0 ? void 0 : pkgJSONOfMonorepoRoot.dependencies) !== null && _b !== void 0 ? _b : {};
            return {
                devDependencies,
                dependencies,
            };
        }
        return {};
    })();
    const appId = (_a = conf.extConfig.appId) !== null && _a !== void 0 ? _a : '';
    const appName = (_b = conf.extConfig.appName) !== null && _b !== void 0 ? _b : '';
    const webDir = (_c = conf.extConfig.webDir) !== null && _c !== void 0 ? _c : 'www';
    const cli = await loadCLIConfig(cliRootDir);
    const config = {
        android: await loadAndroidConfig(appRootDir, conf.extConfig, cli),
        ios: await loadIOSConfig(appRootDir, conf.extConfig),
        web: await loadWebConfig(appRootDir, webDir),
        cli,
        app: {
            rootDir: appRootDir,
            appId,
            appName,
            webDir,
            webDirAbs: (0, path_1.resolve)(appRootDir, webDir),
            package: (_d = (await (0, fn_1.tryFn)(utils_fs_1.readJSON, (0, path_1.resolve)(appRootDir, 'package.json')))) !== null && _d !== void 0 ? _d : {
                name: appName,
                version: '1.0.0',
                ...depsForNx,
            },
            ...conf,
        },
    };
    debug('config: %O', config);
    return config;
}
exports.loadConfig = loadConfig;
async function writeConfig(extConfig, extConfigFilePath) {
    switch ((0, path_1.extname)(extConfigFilePath)) {
        case '.json': {
            await (0, utils_fs_1.writeJSON)(extConfigFilePath, extConfig, { spaces: 2 });
            break;
        }
        case '.ts': {
            await (0, utils_fs_1.writeFile)(extConfigFilePath, formatConfigTS(extConfig));
            break;
        }
    }
}
exports.writeConfig = writeConfig;
async function loadExtConfigTS(rootDir, extConfigName, extConfigFilePath) {
    var _a;
    try {
        const tsPath = (0, node_1.resolveNode)(rootDir, 'typescript');
        if (!tsPath) {
            (0, errors_1.fatal)('Could not find installation of TypeScript.\n' +
                `To use ${colors_1.default.strong(extConfigName)} files, you must install TypeScript in your project, e.g. w/ ${colors_1.default.input('npm install -D typescript')}`);
        }
        const ts = require(tsPath); // eslint-disable-line @typescript-eslint/no-var-requires
        const extConfigObject = (0, node_1.requireTS)(ts, extConfigFilePath);
        const extConfig = extConfigObject.default
            ? await extConfigObject.default
            : extConfigObject;
        return {
            extConfigType: 'ts',
            extConfigName,
            extConfigFilePath: extConfigFilePath,
            extConfig,
        };
    }
    catch (e) {
        if (!(0, errors_1.isFatal)(e)) {
            (0, errors_1.fatal)(`Parsing ${colors_1.default.strong(extConfigName)} failed.\n\n${(_a = e.stack) !== null && _a !== void 0 ? _a : e}`);
        }
        throw e;
    }
}
async function loadExtConfigJS(rootDir, extConfigName, extConfigFilePath) {
    var _a;
    try {
        return {
            extConfigType: 'js',
            extConfigName,
            extConfigFilePath: extConfigFilePath,
            extConfig: await require(extConfigFilePath),
        };
    }
    catch (e) {
        (0, errors_1.fatal)(`Parsing ${colors_1.default.strong(extConfigName)} failed.\n\n${(_a = e.stack) !== null && _a !== void 0 ? _a : e}`);
    }
}
async function loadExtConfig(rootDir) {
    var _a;
    const extConfigFilePathTS = (0, path_1.resolve)(rootDir, exports.CONFIG_FILE_NAME_TS);
    if (await (0, utils_fs_1.pathExists)(extConfigFilePathTS)) {
        return loadExtConfigTS(rootDir, exports.CONFIG_FILE_NAME_TS, extConfigFilePathTS);
    }
    const extConfigFilePathJS = (0, path_1.resolve)(rootDir, exports.CONFIG_FILE_NAME_JS);
    if (await (0, utils_fs_1.pathExists)(extConfigFilePathJS)) {
        return loadExtConfigJS(rootDir, exports.CONFIG_FILE_NAME_JS, extConfigFilePathJS);
    }
    const extConfigFilePath = (0, path_1.resolve)(rootDir, exports.CONFIG_FILE_NAME_JSON);
    return {
        extConfigType: 'json',
        extConfigName: exports.CONFIG_FILE_NAME_JSON,
        extConfigFilePath: extConfigFilePath,
        extConfig: (_a = (await (0, fn_1.tryFn)(utils_fs_1.readJSON, extConfigFilePath))) !== null && _a !== void 0 ? _a : {},
    };
}
async function loadCLIConfig(rootDir) {
    const assetsDir = 'assets';
    const assetsDirAbs = (0, path_1.join)(rootDir, assetsDir);
    const iosPlatformTemplateArchive = 'ios-pods-template.tar.gz';
    const iosCordovaPluginsTemplateArchive = 'capacitor-cordova-ios-plugins.tar.gz';
    const androidPlatformTemplateArchive = 'android-template.tar.gz';
    const androidCordovaPluginsTemplateArchive = 'capacitor-cordova-android-plugins.tar.gz';
    return {
        rootDir,
        assetsDir,
        assetsDirAbs,
        assets: {
            ios: {
                platformTemplateArchive: iosPlatformTemplateArchive,
                platformTemplateArchiveAbs: (0, path_1.resolve)(assetsDirAbs, iosPlatformTemplateArchive),
                cordovaPluginsTemplateArchive: iosCordovaPluginsTemplateArchive,
                cordovaPluginsTemplateArchiveAbs: (0, path_1.resolve)(assetsDirAbs, iosCordovaPluginsTemplateArchive),
            },
            android: {
                platformTemplateArchive: androidPlatformTemplateArchive,
                platformTemplateArchiveAbs: (0, path_1.resolve)(assetsDirAbs, androidPlatformTemplateArchive),
                cordovaPluginsTemplateArchive: androidCordovaPluginsTemplateArchive,
                cordovaPluginsTemplateArchiveAbs: (0, path_1.resolve)(assetsDirAbs, androidCordovaPluginsTemplateArchive),
            },
        },
        package: await (0, utils_fs_1.readJSON)((0, path_1.resolve)(rootDir, 'package.json')),
        os: determineOS(process.platform),
    };
}
async function loadAndroidConfig(rootDir, extConfig, cliConfig) {
    var _a, _b, _c, _d, _e, _f, _g, _h, _j, _k, _l, _m, _o, _p, _q, _r, _s;
    const name = 'android';
    const platformDir = (_b = (_a = extConfig.android) === null || _a === void 0 ? void 0 : _a.path) !== null && _b !== void 0 ? _b : 'android';
    const platformDirAbs = (0, path_1.resolve)(rootDir, platformDir);
    const appDir = 'app';
    const srcDir = `${appDir}/src`;
    const srcMainDir = `${srcDir}/main`;
    const assetsDir = `${srcMainDir}/assets`;
    const webDir = `${assetsDir}/public`;
    const resDir = `${srcMainDir}/res`;
    let apkPath = `${appDir}/build/outputs/apk/`;
    const flavor = ((_c = extConfig.android) === null || _c === void 0 ? void 0 : _c.flavor) || '';
    if ((_d = extConfig.android) === null || _d === void 0 ? void 0 : _d.flavor) {
        apkPath = `${apkPath}/${(_e = extConfig.android) === null || _e === void 0 ? void 0 : _e.flavor}`;
    }
    const apkName = (0, common_1.parseApkNameFromFlavor)(flavor);
    const buildOutputDir = `${apkPath}/debug`;
    const cordovaPluginsDir = 'capacitor-cordova-android-plugins';
    const studioPath = (0, promise_1.lazy)(() => determineAndroidStudioPath(cliConfig.os));
    const buildOptions = {
        keystorePath: (_g = (_f = extConfig.android) === null || _f === void 0 ? void 0 : _f.buildOptions) === null || _g === void 0 ? void 0 : _g.keystorePath,
        keystorePassword: (_j = (_h = extConfig.android) === null || _h === void 0 ? void 0 : _h.buildOptions) === null || _j === void 0 ? void 0 : _j.keystorePassword,
        keystoreAlias: (_l = (_k = extConfig.android) === null || _k === void 0 ? void 0 : _k.buildOptions) === null || _l === void 0 ? void 0 : _l.keystoreAlias,
        keystoreAliasPassword: (_o = (_m = extConfig.android) === null || _m === void 0 ? void 0 : _m.buildOptions) === null || _o === void 0 ? void 0 : _o.keystoreAliasPassword,
        signingType: (_q = (_p = extConfig.android) === null || _p === void 0 ? void 0 : _p.buildOptions) === null || _q === void 0 ? void 0 : _q.signingType,
        releaseType: (_s = (_r = extConfig.android) === null || _r === void 0 ? void 0 : _r.buildOptions) === null || _s === void 0 ? void 0 : _s.releaseType,
    };
    return {
        name,
        minVersion: '22',
        studioPath,
        platformDir,
        platformDirAbs,
        cordovaPluginsDir,
        cordovaPluginsDirAbs: (0, path_1.resolve)(platformDirAbs, cordovaPluginsDir),
        appDir,
        appDirAbs: (0, path_1.resolve)(platformDirAbs, appDir),
        srcDir,
        srcDirAbs: (0, path_1.resolve)(platformDirAbs, srcDir),
        srcMainDir,
        srcMainDirAbs: (0, path_1.resolve)(platformDirAbs, srcMainDir),
        assetsDir,
        assetsDirAbs: (0, path_1.resolve)(platformDirAbs, assetsDir),
        webDir,
        webDirAbs: (0, path_1.resolve)(platformDirAbs, webDir),
        resDir,
        resDirAbs: (0, path_1.resolve)(platformDirAbs, resDir),
        apkName,
        buildOutputDir,
        buildOutputDirAbs: (0, path_1.resolve)(platformDirAbs, buildOutputDir),
        flavor,
        buildOptions,
    };
}
async function loadIOSConfig(rootDir, extConfig) {
    var _a, _b, _c, _d;
    const name = 'ios';
    const platformDir = (_b = (_a = extConfig.ios) === null || _a === void 0 ? void 0 : _a.path) !== null && _b !== void 0 ? _b : 'ios';
    const platformDirAbs = (0, path_1.resolve)(rootDir, platformDir);
    const scheme = (_d = (_c = extConfig.ios) === null || _c === void 0 ? void 0 : _c.scheme) !== null && _d !== void 0 ? _d : 'App';
    const nativeProjectDir = 'App';
    const nativeProjectDirAbs = (0, path_1.resolve)(platformDirAbs, nativeProjectDir);
    const nativeTargetDir = `${nativeProjectDir}/App`;
    const nativeTargetDirAbs = (0, path_1.resolve)(platformDirAbs, nativeTargetDir);
    const nativeXcodeProjDir = `${nativeProjectDir}/App.xcodeproj`;
    const nativeXcodeProjDirAbs = (0, path_1.resolve)(platformDirAbs, nativeXcodeProjDir);
    const nativeXcodeWorkspaceDirAbs = (0, promise_1.lazy)(() => determineXcodeWorkspaceDirAbs(nativeProjectDirAbs));
    const podPath = (0, promise_1.lazy)(() => determineGemfileOrCocoapodPath(rootDir, platformDirAbs, nativeProjectDirAbs));
    const webDirAbs = (0, promise_1.lazy)(() => determineIOSWebDirAbs(nativeProjectDirAbs, nativeTargetDirAbs, nativeXcodeProjDirAbs));
    const cordovaPluginsDir = 'capacitor-cordova-ios-plugins';
    return {
        name,
        minVersion: '13.0',
        platformDir,
        platformDirAbs,
        scheme,
        cordovaPluginsDir,
        cordovaPluginsDirAbs: (0, path_1.resolve)(platformDirAbs, cordovaPluginsDir),
        nativeProjectDir,
        nativeProjectDirAbs,
        nativeTargetDir,
        nativeTargetDirAbs,
        nativeXcodeProjDir,
        nativeXcodeProjDirAbs,
        nativeXcodeWorkspaceDir: (0, promise_1.lazy)(async () => (0, path_1.relative)(platformDirAbs, await nativeXcodeWorkspaceDirAbs)),
        nativeXcodeWorkspaceDirAbs,
        webDir: (0, promise_1.lazy)(async () => (0, path_1.relative)(platformDirAbs, await webDirAbs)),
        webDirAbs,
        podPath,
    };
}
async function loadWebConfig(rootDir, webDir) {
    const platformDir = webDir;
    const platformDirAbs = (0, path_1.resolve)(rootDir, platformDir);
    return {
        name: 'web',
        platformDir,
        platformDirAbs,
    };
}
function determineOS(os) {
    switch (os) {
        case 'darwin':
            return "mac" /* OS.Mac */;
        case 'win32':
            return "windows" /* OS.Windows */;
        case 'linux':
            return "linux" /* OS.Linux */;
    }
    return "unknown" /* OS.Unknown */;
}
async function determineXcodeWorkspaceDirAbs(nativeProjectDirAbs) {
    return (0, path_1.resolve)(nativeProjectDirAbs, 'App.xcworkspace');
}
async function determineIOSWebDirAbs(nativeProjectDirAbs, nativeTargetDirAbs, nativeXcodeProjDirAbs) {
    const re = /path\s=\spublic[\s\S]+?sourceTree\s=\s([^;]+)/;
    const pbxprojPath = (0, path_1.resolve)(nativeXcodeProjDirAbs, 'project.pbxproj');
    try {
        const pbxproj = await (0, utils_fs_1.readFile)(pbxprojPath, { encoding: 'utf8' });
        const m = pbxproj.match(re);
        if (m && m[1] === 'SOURCE_ROOT') {
            log_1.logger.warn(`Using the iOS project root for the ${colors_1.default.strong('public')} directory is deprecated.\n` +
                `Please follow the Upgrade Guide to move ${colors_1.default.strong('public')} inside the iOS target directory: ${colors_1.default.strong('https://capacitorjs.com/docs/updating/3-0#move-public-into-the-ios-target-directory')}`);
            return (0, path_1.resolve)(nativeProjectDirAbs, 'public');
        }
    }
    catch (e) {
        // ignore
    }
    return (0, path_1.resolve)(nativeTargetDirAbs, 'public');
}
async function determineAndroidStudioPath(os) {
    if (process.env.CAPACITOR_ANDROID_STUDIO_PATH) {
        return process.env.CAPACITOR_ANDROID_STUDIO_PATH;
    }
    switch (os) {
        case "mac" /* OS.Mac */:
            return '/Applications/Android Studio.app';
        case "windows" /* OS.Windows */: {
            const { runCommand } = await Promise.resolve().then(() => tslib_1.__importStar(require('./util/subprocess')));
            let p = 'C:\\Program Files\\Android\\Android Studio\\bin\\studio64.exe';
            try {
                if (!(await (0, utils_fs_1.pathExists)(p))) {
                    let commandResult = await runCommand('REG', [
                        'QUERY',
                        'HKEY_LOCAL_MACHINE\\SOFTWARE\\Android Studio',
                        '/v',
                        'Path',
                    ]);
                    commandResult = commandResult.replace(/(\r\n|\n|\r)/gm, '');
                    const i = commandResult.indexOf('REG_SZ');
                    if (i > 0) {
                        p = commandResult.substring(i + 6).trim() + '\\bin\\studio64.exe';
                    }
                }
            }
            catch (e) {
                debug(`Error checking registry for Android Studio path: %O`, e);
                break;
            }
            return p;
        }
        case "linux" /* OS.Linux */:
            return '/usr/local/android-studio/bin/studio.sh';
    }
    return '';
}
async function determineGemfileOrCocoapodPath(rootDir, platformDir, nativeProjectDirAbs) {
    if (process.env.CAPACITOR_COCOAPODS_PATH) {
        return process.env.CAPACITOR_COCOAPODS_PATH;
    }
    let gemfilePath = '';
    if (await (0, utils_fs_1.pathExists)((0, path_1.resolve)(rootDir, 'Gemfile'))) {
        gemfilePath = (0, path_1.resolve)(rootDir, 'Gemfile');
    }
    else if (await (0, utils_fs_1.pathExists)((0, path_1.resolve)(platformDir, 'Gemfile'))) {
        gemfilePath = (0, path_1.resolve)(platformDir, 'Gemfile');
    }
    else if (await (0, utils_fs_1.pathExists)((0, path_1.resolve)(nativeProjectDirAbs, 'Gemfile'))) {
        gemfilePath = (0, path_1.resolve)(nativeProjectDirAbs, 'Gemfile');
    }
    const appSpecificGemfileExists = gemfilePath != '';
    // Multi-app projects might share a single global 'Gemfile' at the Git repository root directory.
    if (!appSpecificGemfileExists) {
        try {
            const output = await (0, subprocess_1.getCommandOutput)('git', ['rev-parse', '--show-toplevel'], { cwd: rootDir });
            if (output != null) {
                gemfilePath = (0, path_1.resolve)(output, 'Gemfile');
            }
        }
        catch (e) {
            // Nothing
        }
    }
    try {
        const gemfileText = (await (0, utils_fs_1.readFile)(gemfilePath)).toString();
        if (!gemfileText) {
            return 'pod';
        }
        const cocoapodsInGemfile = new RegExp(/gem\s+['"]cocoapods/).test(gemfileText);
        if (cocoapodsInGemfile) {
            return 'bundle exec pod';
        }
        else {
            return 'pod';
        }
    }
    catch {
        return 'pod';
    }
}
function formatConfigTS(extConfig) {
    // TODO: <reference> tags
    return `import type { CapacitorConfig } from '@capacitor/cli';

const config: CapacitorConfig = ${(0, js_1.formatJSObject)(extConfig)};

export default config;\n`;
}
function checkExternalConfig(config) {
    if (typeof config.extConfig.bundledWebRuntime !== 'undefined') {
        let actionMessage = `Can be safely deleted.`;
        if (config.extConfig.bundledWebRuntime === true) {
            actionMessage = `Please, use a bundler to bundle Capacitor and its plugins.`;
        }
        log_1.logger.warn(`The ${colors_1.default.strong('bundledWebRuntime')} configuration option has been deprecated. ${actionMessage}`);
    }
}
exports.checkExternalConfig = checkExternalConfig;
