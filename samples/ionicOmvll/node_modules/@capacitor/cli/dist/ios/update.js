"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.installCocoaPodsPlugins = exports.updateIOS = void 0;
const tslib_1 = require("tslib");
const utils_fs_1 = require("@ionic/utils-fs");
const path_1 = require("path");
const colors_1 = tslib_1.__importDefault(require("../colors"));
const common_1 = require("../common");
const cordova_1 = require("../cordova");
const errors_1 = require("../errors");
const log_1 = require("../log");
const plugin_1 = require("../plugin");
const copy_1 = require("../tasks/copy");
const fs_1 = require("../util/fs");
const iosplugin_1 = require("../util/iosplugin");
const node_1 = require("../util/node");
const spm_1 = require("../util/spm");
const subprocess_1 = require("../util/subprocess");
const template_1 = require("../util/template");
const common_2 = require("./common");
const platform = 'ios';
async function updateIOS(config, deployment) {
    const plugins = await getPluginsTask(config);
    const capacitorPlugins = plugins.filter(p => (0, plugin_1.getPluginType)(p, platform) === 0 /* PluginType.Core */);
    if ((await (0, spm_1.checkPackageManager)(config)) === 'SPM') {
        await (0, spm_1.generatePackageFile)(config, capacitorPlugins);
    }
    else {
        await updateIOSCocoaPods(config, plugins, deployment);
    }
    (0, iosplugin_1.generateIOSPackageJSON)(config, plugins);
    (0, plugin_1.printPlugins)(capacitorPlugins, 'ios');
}
exports.updateIOS = updateIOS;
async function updateIOSCocoaPods(config, plugins, deployment) {
    await removePluginsNativeFiles(config);
    const cordovaPlugins = plugins.filter(p => (0, plugin_1.getPluginType)(p, platform) === 1 /* PluginType.Cordova */);
    if (cordovaPlugins.length > 0) {
        await copyPluginsNativeFiles(config, cordovaPlugins);
    }
    if (!(await (0, utils_fs_1.pathExists)(await config.ios.webDirAbs))) {
        await (0, copy_1.copy)(config, platform);
    }
    await (0, cordova_1.handleCordovaPluginsJS)(cordovaPlugins, config, platform);
    await (0, cordova_1.checkPluginDependencies)(plugins, platform);
    await generateCordovaPodspecs(cordovaPlugins, config);
    await installCocoaPodsPlugins(config, plugins, deployment);
    await (0, cordova_1.logCordovaManualSteps)(cordovaPlugins, config, platform);
    const incompatibleCordovaPlugins = plugins.filter(p => (0, plugin_1.getPluginType)(p, platform) === 2 /* PluginType.Incompatible */);
    (0, plugin_1.printPlugins)(incompatibleCordovaPlugins, platform, 'incompatible');
    await (0, common_1.checkPlatformVersions)(config, platform);
}
async function installCocoaPodsPlugins(config, plugins, deployment) {
    await (0, common_1.runTask)(`Updating iOS native dependencies with ${colors_1.default.input(`${await config.ios.podPath} install`)}`, () => {
        return updatePodfile(config, plugins, deployment);
    });
}
exports.installCocoaPodsPlugins = installCocoaPodsPlugins;
async function updatePodfile(config, plugins, deployment) {
    const dependenciesContent = await generatePodFile(config, plugins);
    const relativeCapacitoriOSPath = await getRelativeCapacitoriOSPath(config);
    const podfilePath = (0, path_1.join)(config.ios.nativeProjectDirAbs, 'Podfile');
    let podfileContent = await (0, utils_fs_1.readFile)(podfilePath, { encoding: 'utf-8' });
    podfileContent = podfileContent.replace(/(def capacitor_pods)[\s\S]+?(\nend)/, `$1${dependenciesContent}$2`);
    podfileContent = podfileContent.replace(/(require_relative)[\s\S]+?(@capacitor\/ios\/scripts\/pods_helpers')/, `require_relative '${relativeCapacitoriOSPath}/scripts/pods_helpers'`);
    podfileContent = podfileContent.replace(`def assertDeploymentTarget(installer)
  installer.pods_project.targets.each do |target|
    target.build_configurations.each do |config|
      # ensure IPHONEOS_DEPLOYMENT_TARGET is at least 13.0
      deployment_target = config.build_settings['IPHONEOS_DEPLOYMENT_TARGET'].to_f
      should_upgrade = deployment_target < 13.0 && deployment_target != 0.0
      if should_upgrade
        config.build_settings['IPHONEOS_DEPLOYMENT_TARGET'] = '13.0'
      end
    end
  end
end`, `require_relative '${relativeCapacitoriOSPath}/scripts/pods_helpers'`);
    await (0, utils_fs_1.writeFile)(podfilePath, podfileContent, { encoding: 'utf-8' });
    const podPath = await config.ios.podPath;
    const useBundler = podPath.startsWith('bundle');
    const podCommandExists = await (0, subprocess_1.isInstalled)('pod');
    if (useBundler || podCommandExists) {
        if (useBundler) {
            await (0, subprocess_1.runCommand)('bundle', ['exec', 'pod', 'install', ...(deployment ? ['--deployment'] : [])], { cwd: config.ios.nativeProjectDirAbs });
        }
        else {
            await (0, subprocess_1.runCommand)(podPath, ['install', ...(deployment ? ['--deployment'] : [])], { cwd: config.ios.nativeProjectDirAbs });
        }
    }
    else {
        log_1.logger.warn('Skipping pod install because CocoaPods is not installed');
    }
    const isXcodebuildAvailable = await (0, subprocess_1.isInstalled)('xcodebuild');
    if (isXcodebuildAvailable) {
        await (0, subprocess_1.runCommand)('xcodebuild', ['-project', (0, path_1.basename)(`${config.ios.nativeXcodeProjDirAbs}`), 'clean'], {
            cwd: config.ios.nativeProjectDirAbs,
        });
    }
    else {
        log_1.logger.warn('Unable to find "xcodebuild". Skipping xcodebuild clean step...');
    }
}
async function getRelativeCapacitoriOSPath(config) {
    const capacitoriOSPath = (0, node_1.resolveNode)(config.app.rootDir, '@capacitor/ios', 'package.json');
    if (!capacitoriOSPath) {
        (0, errors_1.fatal)(`Unable to find ${colors_1.default.strong('node_modules/@capacitor/ios')}.\n` +
            `Are you sure ${colors_1.default.strong('@capacitor/ios')} is installed?`);
    }
    return (0, fs_1.convertToUnixPath)((0, path_1.relative)(config.ios.nativeProjectDirAbs, await (0, utils_fs_1.realpath)((0, path_1.dirname)(capacitoriOSPath))));
}
async function generatePodFile(config, plugins) {
    const relativeCapacitoriOSPath = await getRelativeCapacitoriOSPath(config);
    const capacitorPlugins = plugins.filter(p => (0, plugin_1.getPluginType)(p, platform) === 0 /* PluginType.Core */);
    const pods = await Promise.all(capacitorPlugins.map(async (p) => {
        if (!p.ios) {
            return '';
        }
        return `  pod '${p.ios.name}', :path => '${(0, fs_1.convertToUnixPath)((0, path_1.relative)(config.ios.nativeProjectDirAbs, await (0, utils_fs_1.realpath)(p.rootPath)))}'\n`;
    }));
    const cordovaPlugins = plugins.filter(p => (0, plugin_1.getPluginType)(p, platform) === 1 /* PluginType.Cordova */);
    cordovaPlugins.map(async (p) => {
        const podspecs = (0, plugin_1.getPlatformElement)(p, platform, 'podspec');
        podspecs.map((podspec) => {
            podspec.pods.map((pPods) => {
                pPods.pod.map((pod) => {
                    if (pod.$.git) {
                        let gitRef = '';
                        if (pod.$.tag) {
                            gitRef = `, :tag => '${pod.$.tag}'`;
                        }
                        else if (pod.$.branch) {
                            gitRef = `, :branch => '${pod.$.branch}'`;
                        }
                        else if (pod.$.commit) {
                            gitRef = `, :commit => '${pod.$.commit}'`;
                        }
                        pods.push(`  pod '${pod.$.name}', :git => '${pod.$.git}'${gitRef}\n`);
                    }
                });
            });
        });
    });
    const staticPlugins = cordovaPlugins.filter(p => (0, cordova_1.needsStaticPod)(p, config));
    const noStaticPlugins = cordovaPlugins.filter(el => !staticPlugins.includes(el));
    if (noStaticPlugins.length > 0) {
        pods.push(`  pod 'CordovaPlugins', :path => '../capacitor-cordova-ios-plugins'\n`);
    }
    if (staticPlugins.length > 0) {
        pods.push(`  pod 'CordovaPluginsStatic', :path => '../capacitor-cordova-ios-plugins'\n`);
    }
    const resourcesPlugins = cordovaPlugins.filter(filterResources);
    if (resourcesPlugins.length > 0) {
        pods.push(`  pod 'CordovaPluginsResources', :path => '../capacitor-cordova-ios-plugins'\n`);
    }
    return `
  pod 'Capacitor', :path => '${relativeCapacitoriOSPath}'
  pod 'CapacitorCordova', :path => '${relativeCapacitoriOSPath}'
${pods.join('').trimRight()}`;
}
function getFrameworkName(framework) {
    if (isFramework(framework)) {
        if (framework.$.custom && framework.$.custom === 'true') {
            return framework.$.src;
        }
        return framework.$.src.substr(0, framework.$.src.indexOf('.'));
    }
    return framework.$.src
        .substr(0, framework.$.src.indexOf('.'))
        .replace('lib', '');
}
function isFramework(framework) {
    return framework.$.src.split('.').pop().includes('framework');
}
async function generateCordovaPodspecs(cordovaPlugins, config) {
    const staticPlugins = cordovaPlugins.filter(p => (0, cordova_1.needsStaticPod)(p, config));
    const noStaticPlugins = cordovaPlugins.filter(el => !staticPlugins.includes(el));
    generateCordovaPodspec(noStaticPlugins, config, false);
    generateCordovaPodspec(staticPlugins, config, true);
}
async function generateCordovaPodspec(cordovaPlugins, config, isStatic) {
    const weakFrameworks = [];
    const linkedFrameworks = [];
    const customFrameworks = [];
    const systemLibraries = [];
    const sourceFrameworks = [];
    const frameworkDeps = [];
    const compilerFlags = [];
    let prefsArray = [];
    let name = 'CordovaPlugins';
    let sourcesFolderName = 'sources';
    if (isStatic) {
        name += 'Static';
        frameworkDeps.push('s.static_framework = true');
        sourcesFolderName += 'static';
    }
    cordovaPlugins.map((plugin) => {
        const frameworks = (0, plugin_1.getPlatformElement)(plugin, platform, 'framework');
        frameworks.map((framework) => {
            if (!framework.$.type) {
                const name = getFrameworkName(framework);
                if (isFramework(framework)) {
                    if (framework.$.weak && framework.$.weak === 'true') {
                        if (!weakFrameworks.includes(name)) {
                            weakFrameworks.push(name);
                        }
                    }
                    else if (framework.$.custom && framework.$.custom === 'true') {
                        const frameworktPath = (0, path_1.join)(sourcesFolderName, plugin.name, name);
                        if (!customFrameworks.includes(frameworktPath)) {
                            customFrameworks.push(frameworktPath);
                        }
                    }
                    else {
                        if (!linkedFrameworks.includes(name)) {
                            linkedFrameworks.push(name);
                        }
                    }
                }
                else {
                    if (!systemLibraries.includes(name)) {
                        systemLibraries.push(name);
                    }
                }
            }
            else if (framework.$.type && framework.$.type === 'podspec') {
                let depString = `s.dependency '${framework.$.src}'`;
                if (framework.$.spec && framework.$.spec !== '') {
                    depString += `, '${framework.$.spec}'`;
                }
                if (!frameworkDeps.includes(depString)) {
                    frameworkDeps.push(depString);
                }
            }
        });
        prefsArray = prefsArray.concat((0, plugin_1.getAllElements)(plugin, platform, 'preference'));
        const podspecs = (0, plugin_1.getPlatformElement)(plugin, platform, 'podspec');
        podspecs.map((podspec) => {
            podspec.pods.map((pods) => {
                pods.pod.map((pod) => {
                    let depString = `s.dependency '${pod.$.name}'`;
                    if (pod.$.spec && pod.$.spec !== '') {
                        depString += `, '${pod.$.spec}'`;
                    }
                    if (!frameworkDeps.includes(depString)) {
                        frameworkDeps.push(depString);
                    }
                });
            });
        });
        const sourceFiles = (0, plugin_1.getPlatformElement)(plugin, platform, 'source-file');
        sourceFiles.map((sourceFile) => {
            if (sourceFile.$.framework && sourceFile.$.framework === 'true') {
                let fileName = sourceFile.$.src.split('/').pop();
                if (!fileName.startsWith('lib')) {
                    fileName = 'lib' + fileName;
                }
                const frameworktPath = (0, path_1.join)(sourcesFolderName, plugin.name, fileName);
                if (!sourceFrameworks.includes(frameworktPath)) {
                    sourceFrameworks.push(frameworktPath);
                }
            }
            else if (sourceFile.$['compiler-flags']) {
                const cFlag = sourceFile.$['compiler-flags'];
                if (!compilerFlags.includes(cFlag)) {
                    compilerFlags.push(cFlag);
                }
            }
        });
    });
    const onlySystemLibraries = systemLibraries.filter(library => removeNoSystem(library, sourceFrameworks));
    if (weakFrameworks.length > 0) {
        frameworkDeps.push(`s.weak_frameworks = '${weakFrameworks.join(`', '`)}'`);
    }
    if (linkedFrameworks.length > 0) {
        frameworkDeps.push(`s.frameworks = '${linkedFrameworks.join(`', '`)}'`);
    }
    if (onlySystemLibraries.length > 0) {
        frameworkDeps.push(`s.libraries = '${onlySystemLibraries.join(`', '`)}'`);
    }
    if (customFrameworks.length > 0) {
        frameworkDeps.push(`s.vendored_frameworks = '${customFrameworks.join(`', '`)}'`);
        frameworkDeps.push(`s.exclude_files = 'sources/**/*.framework/Headers/*.h', 'sources/**/*.framework/PrivateHeaders/*.h'`);
    }
    if (sourceFrameworks.length > 0) {
        frameworkDeps.push(`s.vendored_libraries = '${sourceFrameworks.join(`', '`)}'`);
    }
    if (compilerFlags.length > 0) {
        frameworkDeps.push(`s.compiler_flags = '${compilerFlags.join(' ')}'`);
    }
    const arcPlugins = cordovaPlugins.filter(filterARCFiles);
    if (arcPlugins.length > 0) {
        frameworkDeps.push(`s.subspec 'noarc' do |sna|
      sna.requires_arc = false
      sna.source_files = 'noarc/**/*.{swift,h,m,c,cc,mm,cpp}'
    end`);
    }
    let frameworksString = frameworkDeps.join('\n    ');
    frameworksString = await replaceFrameworkVariables(config, prefsArray, frameworksString);
    const content = `
  Pod::Spec.new do |s|
    s.name = '${name}'
    s.version = '${config.cli.package.version}'
    s.summary = 'Autogenerated spec'
    s.license = 'Unknown'
    s.homepage = 'https://example.com'
    s.authors = { 'Capacitor Generator' => 'hi@example.com' }
    s.source = { :git => 'https://github.com/ionic-team/does-not-exist.git', :tag => '${config.cli.package.version}' }
    s.source_files = '${sourcesFolderName}/**/*.{swift,h,m,c,cc,mm,cpp}'
    s.ios.deployment_target  = '${config.ios.minVersion}'
    s.xcconfig = {'GCC_PREPROCESSOR_DEFINITIONS' => '$(inherited) COCOAPODS=1 WK_WEB_VIEW_ONLY=1' }
    s.dependency 'CapacitorCordova'${getLinkerFlags(config)}
    s.swift_version  = '5.1'
    ${frameworksString}
  end`;
    await (0, utils_fs_1.writeFile)((0, path_1.join)(config.ios.cordovaPluginsDirAbs, `${name}.podspec`), content);
}
function getLinkerFlags(config) {
    var _a;
    if ((_a = config.app.extConfig.ios) === null || _a === void 0 ? void 0 : _a.cordovaLinkerFlags) {
        return `\n    s.pod_target_xcconfig = { 'OTHER_LDFLAGS' => '${config.app.extConfig.ios.cordovaLinkerFlags.join(' ')}' }`;
    }
    return '';
}
async function copyPluginsNativeFiles(config, cordovaPlugins) {
    for (const p of cordovaPlugins) {
        const sourceFiles = (0, plugin_1.getPlatformElement)(p, platform, 'source-file');
        const headerFiles = (0, plugin_1.getPlatformElement)(p, platform, 'header-file');
        const codeFiles = sourceFiles.concat(headerFiles);
        const frameworks = (0, plugin_1.getPlatformElement)(p, platform, 'framework');
        let sourcesFolderName = 'sources';
        if ((0, cordova_1.needsStaticPod)(p, config)) {
            sourcesFolderName += 'static';
        }
        const sourcesFolder = (0, path_1.join)(config.ios.cordovaPluginsDirAbs, sourcesFolderName, p.name);
        for (const codeFile of codeFiles) {
            let fileName = codeFile.$.src.split('/').pop();
            const fileExt = codeFile.$.src.split('.').pop();
            if (fileExt === 'a' && !fileName.startsWith('lib')) {
                fileName = 'lib' + fileName;
            }
            let destFolder = sourcesFolderName;
            if (codeFile.$['compiler-flags'] &&
                codeFile.$['compiler-flags'] === '-fno-objc-arc') {
                destFolder = 'noarc';
            }
            const filePath = (0, plugin_1.getFilePath)(config, p, codeFile.$.src);
            const fileDest = (0, path_1.join)(config.ios.cordovaPluginsDirAbs, destFolder, p.name, fileName);
            await (0, utils_fs_1.copy)(filePath, fileDest);
            if (!codeFile.$.framework) {
                let fileContent = await (0, utils_fs_1.readFile)(fileDest, { encoding: 'utf-8' });
                if (fileExt === 'swift') {
                    fileContent = 'import Cordova\n' + fileContent;
                    await (0, utils_fs_1.writeFile)(fileDest, fileContent, { encoding: 'utf-8' });
                }
                else {
                    if (fileContent.includes('@import Firebase;')) {
                        fileContent = fileContent.replace('@import Firebase;', '#import <Firebase/Firebase.h>');
                        await (0, utils_fs_1.writeFile)(fileDest, fileContent, { encoding: 'utf-8' });
                    }
                    if (fileContent.includes('[NSBundle bundleForClass:[self class]]') ||
                        fileContent.includes('[NSBundle bundleForClass:[CDVCapture class]]')) {
                        fileContent = fileContent.replace('[NSBundle bundleForClass:[self class]]', '[NSBundle mainBundle]');
                        fileContent = fileContent.replace('[NSBundle bundleForClass:[CDVCapture class]]', '[NSBundle mainBundle]');
                        await (0, utils_fs_1.writeFile)(fileDest, fileContent, { encoding: 'utf-8' });
                    }
                    if (fileContent.includes('[self.webView superview]') ||
                        fileContent.includes('self.webView.superview')) {
                        fileContent = fileContent.replace(/\[self.webView superview\]/g, 'self.viewController.view');
                        fileContent = fileContent.replace(/self.webView.superview/g, 'self.viewController.view');
                        await (0, utils_fs_1.writeFile)(fileDest, fileContent, { encoding: 'utf-8' });
                    }
                }
            }
        }
        const resourceFiles = (0, plugin_1.getPlatformElement)(p, platform, 'resource-file');
        for (const resourceFile of resourceFiles) {
            const fileName = resourceFile.$.src.split('/').pop();
            await (0, utils_fs_1.copy)((0, plugin_1.getFilePath)(config, p, resourceFile.$.src), (0, path_1.join)(config.ios.cordovaPluginsDirAbs, 'resources', fileName));
        }
        for (const framework of frameworks) {
            if (framework.$.custom && framework.$.custom === 'true') {
                await (0, utils_fs_1.copy)((0, plugin_1.getFilePath)(config, p, framework.$.src), (0, path_1.join)(sourcesFolder, framework.$.src));
            }
        }
    }
}
async function removePluginsNativeFiles(config) {
    await (0, utils_fs_1.remove)(config.ios.cordovaPluginsDirAbs);
    await (0, template_1.extractTemplate)(config.cli.assets.ios.cordovaPluginsTemplateArchiveAbs, config.ios.cordovaPluginsDirAbs);
}
function filterResources(plugin) {
    const resources = (0, plugin_1.getPlatformElement)(plugin, platform, 'resource-file');
    return resources.length > 0;
}
function filterARCFiles(plugin) {
    const sources = (0, plugin_1.getPlatformElement)(plugin, platform, 'source-file');
    const sourcesARC = sources.filter((sourceFile) => sourceFile.$['compiler-flags'] &&
        sourceFile.$['compiler-flags'] === '-fno-objc-arc');
    return sourcesARC.length > 0;
}
function removeNoSystem(library, sourceFrameworks) {
    const libraries = sourceFrameworks.filter(framework => framework.includes(library));
    return libraries.length === 0;
}
async function getPluginsTask(config) {
    return await (0, common_1.runTask)('Updating iOS plugins', async () => {
        const allPlugins = await (0, plugin_1.getPlugins)(config, 'ios');
        const iosPlugins = await (0, common_2.getIOSPlugins)(allPlugins);
        return iosPlugins;
    });
}
async function replaceFrameworkVariables(config, prefsArray, frameworkString) {
    prefsArray.map((preference) => {
        frameworkString = frameworkString.replace(new RegExp(('$' + preference.$.name).replace('$', '\\$&'), 'g'), preference.$.default);
    });
    return frameworkString;
}
