"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.writeCordovaAndroidManifest = exports.getCordovaPreferences = exports.needsStaticPod = exports.getIncompatibleCordovaPlugins = exports.checkPluginDependencies = exports.logCordovaManualSteps = exports.getCordovaPlugins = exports.handleCordovaPluginsJS = exports.autoGenerateConfig = exports.removePluginFiles = exports.createEmptyCordovaJS = exports.copyCordovaJS = exports.copyPluginsJS = exports.generateCordovaPluginsJSFile = void 0;
const tslib_1 = require("tslib");
const utils_fs_1 = require("@ionic/utils-fs");
const path_1 = require("path");
const plist_1 = tslib_1.__importDefault(require("plist"));
const prompts_1 = tslib_1.__importDefault(require("prompts"));
const common_1 = require("./android/common");
const colors_1 = tslib_1.__importDefault(require("./colors"));
const errors_1 = require("./errors");
const common_2 = require("./ios/common");
const log_1 = require("./log");
const plugin_1 = require("./plugin");
const node_1 = require("./util/node");
const term_1 = require("./util/term");
const xml_1 = require("./util/xml");
/**
 * Build the root cordova_plugins.js file referencing each Plugin JS file.
 */
function generateCordovaPluginsJSFile(config, plugins, platform) {
    const pluginModules = [];
    const pluginExports = [];
    plugins.map(p => {
        const pluginId = p.xml.$.id;
        const jsModules = (0, plugin_1.getJSModules)(p, platform);
        jsModules.map((jsModule) => {
            const clobbers = [];
            const merges = [];
            let clobbersModule = '';
            let mergesModule = '';
            let runsModule = '';
            let clobberKey = '';
            let mergeKey = '';
            if (jsModule.clobbers) {
                jsModule.clobbers.map((clobber) => {
                    clobbers.push(clobber.$.target);
                    clobberKey = clobber.$.target;
                });
                clobbersModule = `,
        "clobbers": [
          "${clobbers.join('",\n          "')}"
        ]`;
            }
            if (jsModule.merges) {
                jsModule.merges.map((merge) => {
                    merges.push(merge.$.target);
                    mergeKey = merge.$.target;
                });
                mergesModule = `,
        "merges": [
          "${merges.join('",\n          "')}"
        ]`;
            }
            if (jsModule.runs) {
                runsModule = ',\n        "runs": true';
            }
            const pluginModule = {
                clobber: clobberKey,
                merge: mergeKey,
                // mimics Cordova's module name logic if the name attr is missing
                pluginContent: `{
          "id": "${pluginId +
                    '.' +
                    (jsModule.$.name || jsModule.$.src.match(/([^/]+)\.js/)[1])}",
          "file": "plugins/${pluginId}/${jsModule.$.src}",
          "pluginId": "${pluginId}"${clobbersModule}${mergesModule}${runsModule}
        }`,
            };
            pluginModules.push(pluginModule);
        });
        pluginExports.push(`"${pluginId}": "${p.xml.$.version}"`);
    });
    return `
  cordova.define('cordova/plugin_list', function(require, exports, module) {
    module.exports = [
      ${pluginModules
        .sort((a, b) => a.clobber && b.clobber // Clobbers in alpha order
        ? a.clobber.localeCompare(b.clobber)
        : a.clobber || b.clobber // Clobbers before anything else
            ? b.clobber.localeCompare(a.clobber)
            : a.merge.localeCompare(b.merge))
        .map(e => e.pluginContent)
        .join(',\n      ')}
    ];
    module.exports.metadata =
    // TOP OF METADATA
    {
      ${pluginExports.join(',\n      ')}
    };
    // BOTTOM OF METADATA
    });
    `;
}
exports.generateCordovaPluginsJSFile = generateCordovaPluginsJSFile;
/**
 * Build the plugins/* files for each Cordova plugin installed.
 */
async function copyPluginsJS(config, cordovaPlugins, platform) {
    const webDir = await getWebDir(config, platform);
    const pluginsDir = (0, path_1.join)(webDir, 'plugins');
    const cordovaPluginsJSFile = (0, path_1.join)(webDir, 'cordova_plugins.js');
    await removePluginFiles(config, platform);
    await Promise.all(cordovaPlugins.map(async (p) => {
        const pluginId = p.xml.$.id;
        const pluginDir = (0, path_1.join)(pluginsDir, pluginId, 'www');
        await (0, utils_fs_1.ensureDir)(pluginDir);
        const jsModules = (0, plugin_1.getJSModules)(p, platform);
        await Promise.all(jsModules.map(async (jsModule) => {
            const filePath = (0, path_1.join)(webDir, 'plugins', pluginId, jsModule.$.src);
            await (0, utils_fs_1.copy)((0, path_1.join)(p.rootPath, jsModule.$.src), filePath);
            let data = await (0, utils_fs_1.readFile)(filePath, { encoding: 'utf-8' });
            data = data.trim();
            // mimics Cordova's module name logic if the name attr is missing
            const name = pluginId +
                '.' +
                (jsModule.$.name ||
                    (0, path_1.basename)(jsModule.$.src, (0, path_1.extname)(jsModule.$.src)));
            data = `cordova.define("${name}", function(require, exports, module) { \n${data}\n});`;
            data = data.replace(/<script\b[^<]*(?:(?!<\/script>)<[^<]*)*<\/script\s*>/gi, '');
            await (0, utils_fs_1.writeFile)(filePath, data, { encoding: 'utf-8' });
        }));
        const assets = (0, plugin_1.getAssets)(p, platform);
        await Promise.all(assets.map(async (asset) => {
            const filePath = (0, path_1.join)(webDir, asset.$.target);
            await (0, utils_fs_1.copy)((0, path_1.join)(p.rootPath, asset.$.src), filePath);
        }));
    }));
    await (0, utils_fs_1.writeFile)(cordovaPluginsJSFile, generateCordovaPluginsJSFile(config, cordovaPlugins, platform));
}
exports.copyPluginsJS = copyPluginsJS;
async function copyCordovaJS(config, platform) {
    const cordovaPath = (0, node_1.resolveNode)(config.app.rootDir, '@capacitor/core', 'cordova.js');
    if (!cordovaPath) {
        (0, errors_1.fatal)(`Unable to find ${colors_1.default.strong('node_modules/@capacitor/core/cordova.js')}.\n` + `Are you sure ${colors_1.default.strong('@capacitor/core')} is installed?`);
    }
    return (0, utils_fs_1.copy)(cordovaPath, (0, path_1.join)(await getWebDir(config, platform), 'cordova.js'));
}
exports.copyCordovaJS = copyCordovaJS;
async function createEmptyCordovaJS(config, platform) {
    const webDir = await getWebDir(config, platform);
    await (0, utils_fs_1.writeFile)((0, path_1.join)(webDir, 'cordova.js'), '');
    await (0, utils_fs_1.writeFile)((0, path_1.join)(webDir, 'cordova_plugins.js'), '');
}
exports.createEmptyCordovaJS = createEmptyCordovaJS;
async function removePluginFiles(config, platform) {
    const webDir = await getWebDir(config, platform);
    const pluginsDir = (0, path_1.join)(webDir, 'plugins');
    const cordovaPluginsJSFile = (0, path_1.join)(webDir, 'cordova_plugins.js');
    await (0, utils_fs_1.remove)(pluginsDir);
    await (0, utils_fs_1.remove)(cordovaPluginsJSFile);
}
exports.removePluginFiles = removePluginFiles;
async function autoGenerateConfig(config, cordovaPlugins, platform) {
    var _a, _b, _c, _d;
    let xmlDir = (0, path_1.join)(config.android.resDirAbs, 'xml');
    const fileName = 'config.xml';
    if (platform === 'ios') {
        xmlDir = config.ios.nativeTargetDirAbs;
    }
    await (0, utils_fs_1.ensureDir)(xmlDir);
    const cordovaConfigXMLFile = (0, path_1.join)(xmlDir, fileName);
    await (0, utils_fs_1.remove)(cordovaConfigXMLFile);
    const pluginEntries = [];
    cordovaPlugins.map(p => {
        const currentPlatform = (0, plugin_1.getPluginPlatform)(p, platform);
        if (currentPlatform) {
            const configFiles = currentPlatform['config-file'];
            if (configFiles) {
                const configXMLEntries = configFiles.filter(function (item) {
                    var _a;
                    return (_a = item.$) === null || _a === void 0 ? void 0 : _a.target.includes(fileName);
                });
                configXMLEntries.map((entry) => {
                    if (entry.feature) {
                        const feature = { feature: entry.feature };
                        pluginEntries.push(feature);
                    }
                });
            }
        }
    });
    let accessOriginString = [];
    if ((_b = (_a = config.app.extConfig) === null || _a === void 0 ? void 0 : _a.cordova) === null || _b === void 0 ? void 0 : _b.accessOrigins) {
        accessOriginString = await Promise.all(config.app.extConfig.cordova.accessOrigins.map(async (host) => {
            return `
  <access origin="${host}" />`;
        }));
    }
    else {
        accessOriginString.push(`<access origin="*" />`);
    }
    const pluginEntriesString = await Promise.all(pluginEntries.map(async (item) => {
        const xmlString = await (0, xml_1.writeXML)(item);
        return xmlString;
    }));
    let pluginPreferencesString = [];
    if ((_d = (_c = config.app.extConfig) === null || _c === void 0 ? void 0 : _c.cordova) === null || _d === void 0 ? void 0 : _d.preferences) {
        pluginPreferencesString = await Promise.all(Object.entries(config.app.extConfig.cordova.preferences).map(async ([key, value]) => {
            return `
  <preference name="${key}" value="${value}" />`;
        }));
    }
    const content = `<?xml version='1.0' encoding='utf-8'?>
<widget version="1.0.0" xmlns="http://www.w3.org/ns/widgets" xmlns:cdv="http://cordova.apache.org/ns/1.0">
  ${accessOriginString.join('')}
  ${pluginEntriesString.join('')}
  ${pluginPreferencesString.join('')}
</widget>`;
    await (0, utils_fs_1.writeFile)(cordovaConfigXMLFile, content);
}
exports.autoGenerateConfig = autoGenerateConfig;
async function getWebDir(config, platform) {
    if (platform === 'ios') {
        return config.ios.webDirAbs;
    }
    if (platform === 'android') {
        return config.android.webDirAbs;
    }
    return '';
}
async function handleCordovaPluginsJS(cordovaPlugins, config, platform) {
    const webDir = await getWebDir(config, platform);
    await (0, utils_fs_1.mkdirp)(webDir);
    if (cordovaPlugins.length > 0) {
        (0, plugin_1.printPlugins)(cordovaPlugins, platform, 'cordova');
        await copyCordovaJS(config, platform);
        await copyPluginsJS(config, cordovaPlugins, platform);
    }
    else {
        await removePluginFiles(config, platform);
        await createEmptyCordovaJS(config, platform);
    }
    await autoGenerateConfig(config, cordovaPlugins, platform);
}
exports.handleCordovaPluginsJS = handleCordovaPluginsJS;
async function getCordovaPlugins(config, platform) {
    const allPlugins = await (0, plugin_1.getPlugins)(config, platform);
    let plugins = [];
    if (platform === config.ios.name) {
        plugins = await (0, common_2.getIOSPlugins)(allPlugins);
    }
    else if (platform === config.android.name) {
        plugins = await (0, common_1.getAndroidPlugins)(allPlugins);
    }
    return plugins.filter(p => (0, plugin_1.getPluginType)(p, platform) === 1 /* PluginType.Cordova */);
}
exports.getCordovaPlugins = getCordovaPlugins;
async function logCordovaManualSteps(cordovaPlugins, config, platform) {
    cordovaPlugins.map(p => {
        const editConfig = (0, plugin_1.getPlatformElement)(p, platform, 'edit-config');
        const configFile = (0, plugin_1.getPlatformElement)(p, platform, 'config-file');
        editConfig.concat(configFile).map(async (configElement) => {
            if (configElement.$ && !configElement.$.target.includes('config.xml')) {
                if (platform === config.ios.name) {
                    if (configElement.$.target.includes('Info.plist')) {
                        logiOSPlist(configElement, config, p);
                    }
                }
            }
        });
    });
}
exports.logCordovaManualSteps = logCordovaManualSteps;
async function logiOSPlist(configElement, config, plugin) {
    var _a, _b;
    let plistPath = (0, path_1.resolve)(config.ios.nativeTargetDirAbs, 'Info.plist');
    if ((_a = config.app.extConfig.ios) === null || _a === void 0 ? void 0 : _a.scheme) {
        plistPath = (0, path_1.resolve)(config.ios.nativeProjectDirAbs, `${(_b = config.app.extConfig.ios) === null || _b === void 0 ? void 0 : _b.scheme}-Info.plist`);
    }
    if (!(await (0, utils_fs_1.pathExists)(plistPath))) {
        plistPath = (0, path_1.resolve)(config.ios.nativeTargetDirAbs, 'Base.lproj', 'Info.plist');
    }
    if (await (0, utils_fs_1.pathExists)(plistPath)) {
        const xmlMeta = await (0, xml_1.readXML)(plistPath);
        const data = await (0, utils_fs_1.readFile)(plistPath, { encoding: 'utf-8' });
        const trimmedPlistData = data.replace(/(\t|\r|\n)/g, '');
        const plistData = plist_1.default.parse(data);
        const dict = xmlMeta.plist.dict.pop();
        if (!dict.key.includes(configElement.$.parent)) {
            let xml = buildConfigFileXml(configElement);
            xml = `<key>${configElement.$.parent}</key>${getConfigFileTagContent(xml)}`;
            log_1.logger.warn(`Configuration required for ${colors_1.default.strong(plugin.id)}.\n` +
                `Add the following to Info.plist:\n` +
                xml);
        }
        else if (configElement.array || configElement.dict) {
            if (configElement.array &&
                configElement.array.length > 0 &&
                configElement.array[0].string) {
                let xml = '';
                configElement.array[0].string.map((element) => {
                    const d = plistData[configElement.$.parent];
                    if (Array.isArray(d) && !d.includes(element)) {
                        xml = xml.concat(`<string>${element}</string>\n`);
                    }
                });
                if (xml.length > 0) {
                    log_1.logger.warn(`Configuration required for ${colors_1.default.strong(plugin.id)}.\n` +
                        `Add the following in the existing ${colors_1.default.strong(configElement.$.parent)} array of your Info.plist:\n` +
                        xml);
                }
            }
            else {
                let xml = buildConfigFileXml(configElement);
                xml = `<key>${configElement.$.parent}</key>${getConfigFileTagContent(xml)}`;
                xml = `<plist version="1.0"><dict>${xml}</dict></plist>`;
                const parseXmlToSearchable = (childElementsObj, arrayToAddTo) => {
                    for (const childElement of childElementsObj) {
                        const childElementName = childElement['#name'];
                        const toAdd = { name: childElementName };
                        if (childElementName === 'key' || childElementName === 'string') {
                            toAdd.value = childElement['_'];
                        }
                        else {
                            if (childElement['$']) {
                                toAdd.attrs = { ...childElement['$'] };
                            }
                            if (childElement['$$']) {
                                toAdd.children = [];
                                parseXmlToSearchable(childElement['$$'], toAdd['children']);
                            }
                        }
                        arrayToAddTo.push(toAdd);
                    }
                };
                const existingElements = (0, xml_1.parseXML)(trimmedPlistData, {
                    explicitChildren: true,
                    trim: true,
                    preserveChildrenOrder: true,
                });
                const parsedExistingElements = [];
                const rootKeyOfExistingElements = Object.keys(existingElements)[0];
                const rootOfExistingElementsToAdd = { name: rootKeyOfExistingElements, children: [] };
                if (existingElements[rootKeyOfExistingElements]['$']) {
                    rootOfExistingElementsToAdd.attrs = {
                        ...existingElements[rootKeyOfExistingElements]['$'],
                    };
                }
                parseXmlToSearchable(existingElements[rootKeyOfExistingElements]['$$'], rootOfExistingElementsToAdd['children']);
                parsedExistingElements.push(rootOfExistingElementsToAdd);
                const requiredElements = (0, xml_1.parseXML)(xml, {
                    explicitChildren: true,
                    trim: true,
                    preserveChildrenOrder: true,
                });
                const parsedRequiredElements = [];
                const rootKeyOfRequiredElements = Object.keys(requiredElements)[0];
                const rootOfRequiredElementsToAdd = { name: rootKeyOfRequiredElements, children: [] };
                if (requiredElements[rootKeyOfRequiredElements]['$']) {
                    rootOfRequiredElementsToAdd.attrs = {
                        ...requiredElements[rootKeyOfRequiredElements]['$'],
                    };
                }
                parseXmlToSearchable(requiredElements[rootKeyOfRequiredElements]['$$'], rootOfRequiredElementsToAdd['children']);
                parsedRequiredElements.push(rootOfRequiredElementsToAdd);
                const doesContainElements = (requiredElementsArray, existingElementsArray) => {
                    for (const requiredElement of requiredElementsArray) {
                        if (requiredElement.name === 'key' ||
                            requiredElement.name === 'string') {
                            let foundMatch = false;
                            for (const existingElement of existingElementsArray) {
                                if (existingElement.name === requiredElement.name &&
                                    (existingElement.value === requiredElement.value ||
                                        /^[$].{1,}$/.test(requiredElement.value.trim()))) {
                                    foundMatch = true;
                                    break;
                                }
                            }
                            if (!foundMatch) {
                                return false;
                            }
                        }
                        else {
                            let foundMatch = false;
                            for (const existingElement of existingElementsArray) {
                                if (existingElement.name === requiredElement.name) {
                                    if ((requiredElement.children !== undefined) ===
                                        (existingElement.children !== undefined)) {
                                        if (doesContainElements(requiredElement.children, existingElement.children)) {
                                            foundMatch = true;
                                            break;
                                        }
                                    }
                                }
                            }
                            if (!foundMatch) {
                                return false;
                            }
                        }
                    }
                    return true;
                };
                if (!doesContainElements(parsedRequiredElements, parsedExistingElements)) {
                    logPossibleMissingItem(configElement, plugin);
                }
            }
        }
    }
    else {
        logPossibleMissingItem(configElement, plugin);
    }
}
function logPossibleMissingItem(configElement, plugin) {
    let xml = buildConfigFileXml(configElement);
    xml = getConfigFileTagContent(xml);
    xml = removeOuterTags(xml);
    log_1.logger.warn(`Configuration might be missing for ${colors_1.default.strong(plugin.id)}.\n` +
        `Add the following to the existing ${colors_1.default.strong(configElement.$.parent)} entry of Info.plist:\n` +
        xml);
}
function buildConfigFileXml(configElement) {
    return (0, xml_1.buildXmlElement)(configElement, 'config-file');
}
function getConfigFileTagContent(str) {
    return str.replace(/<config-file.+">|<\/config-file>/g, '');
}
function removeOuterTags(str) {
    const start = str.indexOf('>') + 1;
    const end = str.lastIndexOf('<');
    return str.substring(start, end);
}
async function checkPluginDependencies(plugins, platform) {
    const pluginDeps = new Map();
    const cordovaPlugins = plugins.filter(p => (0, plugin_1.getPluginType)(p, platform) === 1 /* PluginType.Cordova */);
    const incompatible = plugins.filter(p => (0, plugin_1.getPluginType)(p, platform) === 2 /* PluginType.Incompatible */);
    await Promise.all(cordovaPlugins.map(async (p) => {
        let allDependencies = [];
        allDependencies = allDependencies.concat((0, plugin_1.getPlatformElement)(p, platform, 'dependency'));
        if (p.xml['dependency']) {
            allDependencies = allDependencies.concat(p.xml['dependency']);
        }
        allDependencies = allDependencies.filter((dep) => !getIncompatibleCordovaPlugins(platform).includes(dep.$.id) &&
            incompatible.filter(p => p.id === dep.$.id || p.xml.$.id === dep.$.id)
                .length === 0);
        if (allDependencies) {
            await Promise.all(allDependencies.map(async (dep) => {
                var _a;
                let plugin = dep.$.id;
                let version = dep.$.version;
                if (plugin.includes('@') && plugin.indexOf('@') !== 0) {
                    [plugin, version] = plugin.split('@');
                }
                if (cordovaPlugins.filter(p => p.id === plugin || p.xml.$.id === plugin).length === 0) {
                    if ((_a = dep.$.url) === null || _a === void 0 ? void 0 : _a.startsWith('http')) {
                        plugin = dep.$.url;
                        version = dep.$.commit;
                    }
                    const deps = pluginDeps.get(p.id) || [];
                    deps.push(`${plugin}${version ? colors_1.default.weak(` (${version})`) : ''}`);
                    pluginDeps.set(p.id, deps);
                }
            }));
        }
    }));
    if (pluginDeps.size > 0) {
        let msg = `${colors_1.default.failure(colors_1.default.strong('Plugins are missing dependencies.'))}\n` +
            `Cordova plugin dependencies must be installed in your project (e.g. w/ ${colors_1.default.input('npm install')}).\n`;
        for (const [plugin, deps] of pluginDeps.entries()) {
            msg +=
                `\n  ${colors_1.default.strong(plugin)} is missing dependencies:\n` +
                    deps.map(d => `    - ${d}`).join('\n');
        }
        log_1.logger.warn(`${msg}\n`);
    }
}
exports.checkPluginDependencies = checkPluginDependencies;
function getIncompatibleCordovaPlugins(platform) {
    const pluginList = [
        'cordova-plugin-splashscreen',
        'cordova-plugin-ionic-webview',
        'cordova-plugin-crosswalk-webview',
        'cordova-plugin-wkwebview-engine',
        'cordova-plugin-console',
        'cordova-plugin-music-controls',
        'cordova-plugin-add-swift-support',
        'cordova-plugin-ionic-keyboard',
        'cordova-plugin-braintree',
        '@ionic-enterprise/filesystem',
        '@ionic-enterprise/keyboard',
        '@ionic-enterprise/splashscreen',
        'cordova-support-google-services',
    ];
    if (platform === 'ios') {
        pluginList.push('cordova-plugin-statusbar', '@ionic-enterprise/statusbar', 'SalesforceMobileSDK-CordovaPlugin');
    }
    if (platform === 'android') {
        pluginList.push('cordova-plugin-compat');
    }
    return pluginList;
}
exports.getIncompatibleCordovaPlugins = getIncompatibleCordovaPlugins;
function needsStaticPod(plugin, config) {
    var _a, _b, _c, _d;
    let pluginList = [
        'phonegap-plugin-push',
        '@batch.com/cordova-plugin',
        'onesignal-cordova-plugin',
    ];
    if ((_b = (_a = config.app.extConfig) === null || _a === void 0 ? void 0 : _a.cordova) === null || _b === void 0 ? void 0 : _b.staticPlugins) {
        pluginList = pluginList.concat((_d = (_c = config.app.extConfig) === null || _c === void 0 ? void 0 : _c.cordova) === null || _d === void 0 ? void 0 : _d.staticPlugins);
    }
    return pluginList.includes(plugin.id) || useFrameworks(plugin);
}
exports.needsStaticPod = needsStaticPod;
function useFrameworks(plugin) {
    const podspecs = (0, plugin_1.getPlatformElement)(plugin, 'ios', 'podspec');
    const frameworkPods = podspecs.filter((podspec) => podspec.pods.filter((pods) => pods.$ && pods.$['use-frameworks'] === 'true').length > 0);
    return frameworkPods.length > 0;
}
async function getCordovaPreferences(config) {
    var _a, _b, _c, _d, _e;
    const configXml = (0, path_1.join)(config.app.rootDir, 'config.xml');
    let cordova = {};
    if (await (0, utils_fs_1.pathExists)(configXml)) {
        cordova.preferences = {};
        const xmlMeta = await (0, xml_1.readXML)(configXml);
        if (xmlMeta.widget.preference) {
            xmlMeta.widget.preference.map((pref) => {
                cordova.preferences[pref.$.name] = pref.$.value;
            });
        }
    }
    if (cordova.preferences && Object.keys(cordova.preferences).length > 0) {
        if ((0, term_1.isInteractive)()) {
            const answers = await (0, log_1.logPrompt)(`${colors_1.default.strong(`Cordova preferences can be automatically ported to ${colors_1.default.strong(config.app.extConfigName)}.`)}\n` +
                `Keep in mind: Not all values can be automatically migrated from ${colors_1.default.strong('config.xml')}. There may be more work to do.\n` +
                `More info: ${colors_1.default.strong('https://capacitorjs.com/docs/cordova/migrating-from-cordova-to-capacitor')}`, {
                type: 'confirm',
                name: 'confirm',
                message: `Migrate Cordova preferences from config.xml?`,
                initial: true,
            });
            if (answers.confirm) {
                if ((_b = (_a = config.app.extConfig) === null || _a === void 0 ? void 0 : _a.cordova) === null || _b === void 0 ? void 0 : _b.preferences) {
                    const answers = await (0, prompts_1.default)([
                        {
                            type: 'confirm',
                            name: 'confirm',
                            message: `${config.app.extConfigName} already contains Cordova preferences. Overwrite?`,
                        },
                    ], { onCancel: () => process.exit(1) });
                    if (!answers.confirm) {
                        cordova = (_c = config.app.extConfig) === null || _c === void 0 ? void 0 : _c.cordova;
                    }
                }
            }
            else {
                cordova = (_d = config.app.extConfig) === null || _d === void 0 ? void 0 : _d.cordova;
            }
        }
    }
    else {
        cordova = (_e = config.app.extConfig) === null || _e === void 0 ? void 0 : _e.cordova;
    }
    return cordova;
}
exports.getCordovaPreferences = getCordovaPreferences;
async function writeCordovaAndroidManifest(cordovaPlugins, config, platform) {
    var _a;
    const manifestPath = (0, path_1.join)(config.android.cordovaPluginsDirAbs, 'src', 'main', 'AndroidManifest.xml');
    const rootXMLEntries = [];
    const applicationXMLEntries = [];
    const applicationXMLAttributes = [];
    let prefsArray = [];
    cordovaPlugins.map(async (p) => {
        const editConfig = (0, plugin_1.getPlatformElement)(p, platform, 'edit-config');
        const configFile = (0, plugin_1.getPlatformElement)(p, platform, 'config-file');
        prefsArray = prefsArray.concat((0, plugin_1.getAllElements)(p, platform, 'preference'));
        editConfig.concat(configFile).map(async (configElement) => {
            var _a, _b;
            if (configElement.$ &&
                (((_a = configElement.$.target) === null || _a === void 0 ? void 0 : _a.includes('AndroidManifest.xml')) ||
                    ((_b = configElement.$.file) === null || _b === void 0 ? void 0 : _b.includes('AndroidManifest.xml')))) {
                const keys = Object.keys(configElement).filter(k => k !== '$');
                keys.map(k => {
                    configElement[k].map(async (e) => {
                        const xmlElement = (0, xml_1.buildXmlElement)(e, k);
                        const pathParts = getPathParts(configElement.$.parent || configElement.$.target);
                        if (pathParts.length > 1) {
                            if (pathParts.pop() === 'application') {
                                if (configElement.$.mode &&
                                    configElement.$.mode === 'merge' &&
                                    xmlElement.startsWith('<application')) {
                                    Object.keys(e.$).map((ek) => {
                                        applicationXMLAttributes.push(`${ek}="${e.$[ek]}"`);
                                    });
                                }
                                else if (!applicationXMLEntries.includes(xmlElement) &&
                                    !contains(applicationXMLEntries, xmlElement, k)) {
                                    applicationXMLEntries.push(xmlElement);
                                }
                            }
                            else {
                                const manifestPathOfCapApp = (0, path_1.join)(config.android.appDirAbs, 'src', 'main', 'AndroidManifest.xml');
                                const manifestContentTrimmed = (await (0, utils_fs_1.readFile)(manifestPathOfCapApp))
                                    .toString()
                                    .trim()
                                    .replace(/\n|\t|\r/g, '')
                                    .replace(/[\s]{1,}</g, '<')
                                    .replace(/>[\s]{1,}/g, '>')
                                    .replace(/[\s]{2,}/g, ' ');
                                const requiredManifestContentTrimmed = xmlElement
                                    .trim()
                                    .replace(/\n|\t|\r/g, '')
                                    .replace(/[\s]{1,}</g, '<')
                                    .replace(/>[\s]{1,}/g, '>')
                                    .replace(/[\s]{2,}/g, ' ');
                                const pathPartList = getPathParts(configElement.$.parent || configElement.$.target);
                                const doesXmlManifestContainRequiredInfo = (requiredElements, existingElements, pathTarget) => {
                                    const findElementsToSearchIn = (existingElements, pathTarget) => {
                                        const parts = [...pathTarget];
                                        const elementsToSearchNextIn = [];
                                        for (const existingElement of existingElements) {
                                            if (existingElement.name === pathTarget[0]) {
                                                if (existingElement.children) {
                                                    for (const el of existingElement.children) {
                                                        elementsToSearchNextIn.push(el);
                                                    }
                                                }
                                                else {
                                                    elementsToSearchNextIn.push(existingElement);
                                                }
                                            }
                                        }
                                        if (elementsToSearchNextIn.length === 0) {
                                            return [];
                                        }
                                        else {
                                            parts.splice(0, 1);
                                            if (parts.length <= 0) {
                                                return elementsToSearchNextIn;
                                            }
                                            else {
                                                return findElementsToSearchIn(elementsToSearchNextIn, parts);
                                            }
                                        }
                                    };
                                    const parseXmlToSearchable = (childElementsObj, arrayToAddTo) => {
                                        for (const childElementKey of Object.keys(childElementsObj)) {
                                            for (const occurannceOfElement of childElementsObj[childElementKey]) {
                                                const toAdd = { name: childElementKey };
                                                if (occurannceOfElement['$']) {
                                                    toAdd.attrs = { ...occurannceOfElement['$'] };
                                                }
                                                if (occurannceOfElement['$$']) {
                                                    toAdd.children = [];
                                                    parseXmlToSearchable(occurannceOfElement['$$'], toAdd['children']);
                                                }
                                                arrayToAddTo.push(toAdd);
                                            }
                                        }
                                    };
                                    const doesElementMatch = (requiredElement, existingElement) => {
                                        var _a;
                                        if (requiredElement.name !== existingElement.name) {
                                            return false;
                                        }
                                        if ((requiredElement.attrs !== undefined) !==
                                            (existingElement.attrs !== undefined)) {
                                            return false;
                                        }
                                        else {
                                            if (requiredElement.attrs !== undefined) {
                                                const requiredELementAttrKeys = Object.keys(requiredElement.attrs);
                                                for (const key of requiredELementAttrKeys) {
                                                    if (!/^[$].{1,}$/.test(requiredElement.attrs[key].trim())) {
                                                        if (requiredElement.attrs[key] !==
                                                            existingElement.attrs[key]) {
                                                            return false;
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                        if ((requiredElement.children !== undefined) !==
                                            (existingElement.children !== undefined) &&
                                            ((_a = requiredElement.children) === null || _a === void 0 ? void 0 : _a.length) !== 0) {
                                            return false;
                                        }
                                        else {
                                            if (requiredElement.children !== undefined) {
                                                // each req element is in existing element
                                                for (const requiredElementItem of requiredElement.children) {
                                                    let foundRequiredElement = false;
                                                    for (const existingElementItem of existingElement.children) {
                                                        const foundRequiredElementIn = doesElementMatch(requiredElementItem, existingElementItem);
                                                        if (foundRequiredElementIn) {
                                                            foundRequiredElement = true;
                                                            break;
                                                        }
                                                    }
                                                    if (!foundRequiredElement) {
                                                        return false;
                                                    }
                                                }
                                            }
                                            else {
                                                if (requiredElement.children === undefined &&
                                                    existingElement.children === undefined) {
                                                    return true;
                                                }
                                                else {
                                                    let foundRequiredElement = false;
                                                    for (const existingElementItem of existingElement.children) {
                                                        const foundRequiredElementIn = doesElementMatch(requiredElement, existingElementItem);
                                                        if (foundRequiredElementIn) {
                                                            foundRequiredElement = true;
                                                            break;
                                                        }
                                                    }
                                                    if (!foundRequiredElement) {
                                                        return false;
                                                    }
                                                }
                                            }
                                        }
                                        return true;
                                    };
                                    const parsedExistingElements = [];
                                    const rootKeyOfExistingElements = Object.keys(existingElements)[0];
                                    const rootOfExistingElementsToAdd = { name: rootKeyOfExistingElements, children: [] };
                                    if (existingElements[rootKeyOfExistingElements]['$']) {
                                        rootOfExistingElementsToAdd.attrs = {
                                            ...existingElements[rootKeyOfExistingElements]['$'],
                                        };
                                    }
                                    parseXmlToSearchable(existingElements[rootKeyOfExistingElements]['$$'], rootOfExistingElementsToAdd['children']);
                                    parsedExistingElements.push(rootOfExistingElementsToAdd);
                                    const parsedRequiredElements = [];
                                    const rootKeyOfRequiredElements = Object.keys(requiredElements)[0];
                                    const rootOfRequiredElementsToAdd = { name: rootKeyOfRequiredElements, children: [] };
                                    if (requiredElements[rootKeyOfRequiredElements]['$']) {
                                        rootOfRequiredElementsToAdd.attrs = {
                                            ...requiredElements[rootKeyOfRequiredElements]['$'],
                                        };
                                    }
                                    if (requiredElements[rootKeyOfRequiredElements]['$$'] !==
                                        undefined) {
                                        parseXmlToSearchable(requiredElements[rootKeyOfRequiredElements]['$$'], rootOfRequiredElementsToAdd['children']);
                                    }
                                    parsedRequiredElements.push(rootOfRequiredElementsToAdd);
                                    const elementsToSearch = findElementsToSearchIn(parsedExistingElements, pathTarget);
                                    for (const requiredElement of parsedRequiredElements) {
                                        let foundMatch = false;
                                        for (const existingElement of elementsToSearch) {
                                            const doesContain = doesElementMatch(requiredElement, existingElement);
                                            if (doesContain) {
                                                foundMatch = true;
                                                break;
                                            }
                                        }
                                        if (!foundMatch) {
                                            return false;
                                        }
                                    }
                                    return true;
                                };
                                if (!doesXmlManifestContainRequiredInfo((0, xml_1.parseXML)(requiredManifestContentTrimmed, {
                                    explicitChildren: true,
                                    trim: true,
                                }), (0, xml_1.parseXML)(manifestContentTrimmed, {
                                    explicitChildren: true,
                                    trim: true,
                                }), pathPartList)) {
                                    log_1.logger.warn(`Android Configuration required for ${colors_1.default.strong(p.id)}.\n` +
                                        `Add the following to AndroidManifest.xml:\n` +
                                        xmlElement);
                                }
                            }
                        }
                        else {
                            if (!rootXMLEntries.includes(xmlElement) &&
                                !contains(rootXMLEntries, xmlElement, k)) {
                                rootXMLEntries.push(xmlElement);
                            }
                        }
                    });
                });
            }
        });
    });
    const cleartextString = 'android:usesCleartextTraffic="true"';
    const cleartext = ((_a = config.app.extConfig.server) === null || _a === void 0 ? void 0 : _a.cleartext) &&
        !applicationXMLAttributes.includes(cleartextString)
        ? cleartextString
        : '';
    let content = `<?xml version='1.0' encoding='utf-8'?>
<manifest xmlns:android="http://schemas.android.com/apk/res/android"
xmlns:amazon="http://schemas.amazon.com/apk/res/android">
<application ${applicationXMLAttributes.join('\n')} ${cleartext}>
${applicationXMLEntries.join('\n')}
</application>
${rootXMLEntries.join('\n')}
</manifest>`;
    content = content.replace(new RegExp('$PACKAGE_NAME'.replace('$', '\\$&'), 'g'), '${applicationId}');
    for (const preference of prefsArray) {
        content = content.replace(new RegExp(('$' + preference.$.name).replace('$', '\\$&'), 'g'), preference.$.default);
    }
    if (await (0, utils_fs_1.pathExists)(manifestPath)) {
        await (0, utils_fs_1.writeFile)(manifestPath, content);
    }
}
exports.writeCordovaAndroidManifest = writeCordovaAndroidManifest;
function getPathParts(path) {
    const rootPath = 'manifest';
    path = path.replace('/*', rootPath);
    const parts = path.split('/').filter(part => part !== '');
    if (parts.length > 1 || parts.includes(rootPath)) {
        return parts;
    }
    return [rootPath, path];
}
function contains(entries, obj, k) {
    const element = (0, xml_1.parseXML)(obj);
    for (const entry of entries) {
        const current = (0, xml_1.parseXML)(entry);
        if (element &&
            current &&
            current[k] &&
            element[k] &&
            current[k].$ &&
            element[k].$ &&
            element[k].$['android:name'] === current[k].$['android:name']) {
            return true;
        }
    }
    return false;
}
