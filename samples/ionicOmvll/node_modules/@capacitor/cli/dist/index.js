"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.runProgram = exports.run = void 0;
const tslib_1 = require("tslib");
const commander_1 = require("commander");
const path_1 = require("path");
const colors_1 = tslib_1.__importDefault(require("./colors"));
const config_1 = require("./config");
const errors_1 = require("./errors");
const ipc_1 = require("./ipc");
const log_1 = require("./log");
const telemetry_1 = require("./telemetry");
const cli_1 = require("./util/cli");
const emoji_1 = require("./util/emoji");
process.on('unhandledRejection', error => {
    console.error(colors_1.default.failure('[fatal]'), error);
});
process.on('message', ipc_1.receive);
async function run() {
    try {
        const config = await (0, config_1.loadConfig)();
        runProgram(config);
    }
    catch (e) {
        process.exitCode = (0, errors_1.isFatal)(e) ? e.exitCode : 1;
        log_1.logger.error(e.message ? e.message : String(e));
    }
}
exports.run = run;
function runProgram(config) {
    commander_1.program.version(config.cli.package.version);
    commander_1.program
        .command('config', { hidden: true })
        .description(`print evaluated Capacitor config`)
        .option('--json', 'Print in JSON format')
        .action((0, cli_1.wrapAction)(async ({ json }) => {
        const { configCommand } = await Promise.resolve().then(() => tslib_1.__importStar(require('./tasks/config')));
        await configCommand(config, json);
    }));
    commander_1.program
        .command('create [directory] [name] [id]', { hidden: true })
        .description('Creates a new Capacitor project')
        .action((0, cli_1.wrapAction)(async () => {
        const { createCommand } = await Promise.resolve().then(() => tslib_1.__importStar(require('./tasks/create')));
        await createCommand();
    }));
    commander_1.program
        .command('init [appName] [appId]')
        .description(`Initialize Capacitor configuration`)
        .option('--web-dir <value>', 'Optional: Directory of your projects built web assets')
        .action((0, cli_1.wrapAction)((0, telemetry_1.telemetryAction)(config, async (appName, appId, { webDir }) => {
        const { initCommand } = await Promise.resolve().then(() => tslib_1.__importStar(require('./tasks/init')));
        await initCommand(config, appName, appId, webDir);
    })));
    commander_1.program
        .command('serve', { hidden: true })
        .description('Serves a Capacitor Progressive Web App in the browser')
        .action((0, cli_1.wrapAction)(async () => {
        const { serveCommand } = await Promise.resolve().then(() => tslib_1.__importStar(require('./tasks/serve')));
        await serveCommand();
    }));
    commander_1.program
        .command('sync [platform]')
        .description(`${colors_1.default.input('copy')} + ${colors_1.default.input('update')}`)
        .option('--deployment', 'Optional: if provided, pod install will use --deployment option')
        .option('--inline', 'Optional: if true, all source maps will be inlined for easier debugging on mobile devices', false)
        .action((0, cli_1.wrapAction)((0, telemetry_1.telemetryAction)(config, async (platform, { deployment, inline }) => {
        (0, config_1.checkExternalConfig)(config.app);
        const { syncCommand } = await Promise.resolve().then(() => tslib_1.__importStar(require('./tasks/sync')));
        await syncCommand(config, platform, deployment, inline);
    })));
    commander_1.program
        .command('update [platform]')
        .description(`updates the native plugins and dependencies based on ${colors_1.default.strong('package.json')}`)
        .option('--deployment', 'Optional: if provided, pod install will use --deployment option')
        .action((0, cli_1.wrapAction)((0, telemetry_1.telemetryAction)(config, async (platform, { deployment }) => {
        (0, config_1.checkExternalConfig)(config.app);
        const { updateCommand } = await Promise.resolve().then(() => tslib_1.__importStar(require('./tasks/update')));
        await updateCommand(config, platform, deployment);
    })));
    commander_1.program
        .command('copy [platform]')
        .description('copies the web app build into the native app')
        .option('--inline', 'Optional: if true, all source maps will be inlined for easier debugging on mobile devices', false)
        .action((0, cli_1.wrapAction)((0, telemetry_1.telemetryAction)(config, async (platform, { inline }) => {
        (0, config_1.checkExternalConfig)(config.app);
        const { copyCommand } = await Promise.resolve().then(() => tslib_1.__importStar(require('./tasks/copy')));
        await copyCommand(config, platform, inline);
    })));
    commander_1.program
        .command('build <platform>')
        .description('builds the release version of the selected platform')
        .option('--scheme <schemeToBuild>', 'iOS Scheme to build')
        .option('--flavor <flavorToBuild>', 'Android Flavor to build')
        .option('--keystorepath <keystorePath>', 'Path to the keystore')
        .option('--keystorepass <keystorePass>', 'Password to the keystore')
        .option('--keystorealias <keystoreAlias>', 'Key Alias in the keystore')
        .option('--configuration <name>', 'Configuration name of the iOS Scheme')
        .option('--keystorealiaspass <keystoreAliasPass>', 'Password for the Key Alias')
        .addOption(new commander_1.Option('--androidreleasetype <androidreleasetype>', 'Android release type; APK or AAB').choices(['AAB', 'APK']))
        .addOption(new commander_1.Option('--signing-type <signingtype>', 'Program used to sign apps (default: jarsigner)').choices(['apksigner', 'jarsigner']))
        .action((0, cli_1.wrapAction)((0, telemetry_1.telemetryAction)(config, async (platform, { scheme, flavor, keystorepath, keystorepass, keystorealias, keystorealiaspass, androidreleasetype, signingType, configuration, }) => {
        const { buildCommand } = await Promise.resolve().then(() => tslib_1.__importStar(require('./tasks/build')));
        await buildCommand(config, platform, {
            scheme,
            flavor,
            keystorepath,
            keystorepass,
            keystorealias,
            keystorealiaspass,
            androidreleasetype,
            signingtype: signingType,
            configuration,
        });
    })));
    commander_1.program
        .command(`run [platform]`)
        .description(`runs ${colors_1.default.input('sync')}, then builds and deploys the native app`)
        .option('--scheme <schemeName>', 'set the scheme of the iOS project')
        .option('--flavor <flavorName>', 'set the flavor of the Android project (flavor dimensions not yet supported)')
        .option('--list', 'list targets, then quit')
        // TODO: remove once --json is a hidden option (https://github.com/tj/commander.js/issues/1106)
        .allowUnknownOption(true)
        .option('--target <id>', 'use a specific target')
        .option('--no-sync', `do not run ${colors_1.default.input('sync')}`)
        .option('--forwardPorts <port:port>', 'Automatically run "adb reverse" for better live-reloading support')
        .option('-l, --live-reload', 'Enable Live Reload')
        .option('--host <host>', 'Host used for live reload')
        .option('--port <port>', 'Port used for live reload')
        .option('--configuration <name>', 'Configuration name of the iOS Scheme')
        .action((0, cli_1.wrapAction)((0, telemetry_1.telemetryAction)(config, async (platform, { scheme, flavor, list, target, sync, forwardPorts, liveReload, host, port, configuration, }) => {
        const { runCommand } = await Promise.resolve().then(() => tslib_1.__importStar(require('./tasks/run')));
        await runCommand(config, platform, {
            scheme,
            flavor,
            list,
            target,
            sync,
            forwardPorts,
            liveReload,
            host,
            port,
            configuration,
        });
    })));
    commander_1.program
        .command('open [platform]')
        .description('opens the native project workspace (Xcode for iOS)')
        .action((0, cli_1.wrapAction)((0, telemetry_1.telemetryAction)(config, async (platform) => {
        const { openCommand } = await Promise.resolve().then(() => tslib_1.__importStar(require('./tasks/open')));
        await openCommand(config, platform);
    })));
    commander_1.program
        .command('add [platform]')
        .description('add a native platform project')
        .option('--packagemanager <packageManager>', 'The package manager to use for dependency installs (Cocoapods, SPM **experimental**)')
        .action((0, cli_1.wrapAction)((0, telemetry_1.telemetryAction)(config, async (platform, { packagemanager }) => {
        (0, config_1.checkExternalConfig)(config.app);
        const { addCommand } = await Promise.resolve().then(() => tslib_1.__importStar(require('./tasks/add')));
        const configWritable = config;
        if (packagemanager === 'SPM') {
            configWritable.cli.assets.ios.platformTemplateArchive =
                'ios-spm-template.tar.gz';
            configWritable.cli.assets.ios.platformTemplateArchiveAbs = (0, path_1.resolve)(configWritable.cli.assetsDirAbs, configWritable.cli.assets.ios.platformTemplateArchive);
        }
        await addCommand(configWritable, platform);
    })));
    commander_1.program
        .command('ls [platform]')
        .description('list installed Cordova and Capacitor plugins')
        .action((0, cli_1.wrapAction)((0, telemetry_1.telemetryAction)(config, async (platform) => {
        (0, config_1.checkExternalConfig)(config.app);
        const { listCommand } = await Promise.resolve().then(() => tslib_1.__importStar(require('./tasks/list')));
        await listCommand(config, platform);
    })));
    commander_1.program
        .command('doctor [platform]')
        .description('checks the current setup for common errors')
        .action((0, cli_1.wrapAction)((0, telemetry_1.telemetryAction)(config, async (platform) => {
        (0, config_1.checkExternalConfig)(config.app);
        const { doctorCommand } = await Promise.resolve().then(() => tslib_1.__importStar(require('./tasks/doctor')));
        await doctorCommand(config, platform);
    })));
    commander_1.program
        .command('telemetry [on|off]', { hidden: true })
        .description('enable or disable telemetry')
        .action((0, cli_1.wrapAction)(async (onOrOff) => {
        const { telemetryCommand } = await Promise.resolve().then(() => tslib_1.__importStar(require('./tasks/telemetry')));
        await telemetryCommand(onOrOff);
    }));
    commander_1.program
        .command('📡', { hidden: true })
        .description('IPC receiver command')
        .action(() => {
        // no-op: IPC messages are received via `process.on('message')`
    });
    commander_1.program
        .command('plugin:generate', { hidden: true })
        .description('start a new Capacitor plugin')
        .action((0, cli_1.wrapAction)(async () => {
        const { newPluginCommand } = await Promise.resolve().then(() => tslib_1.__importStar(require('./tasks/new-plugin')));
        await newPluginCommand();
    }));
    commander_1.program
        .command('migrate')
        .option('--noprompt', 'do not prompt for confirmation')
        .option('--packagemanager <packageManager>', 'The package manager to use for dependency installs (npm, pnpm, yarn)')
        .description('Migrate your current Capacitor app to the latest major version of Capacitor.')
        .action((0, cli_1.wrapAction)(async ({ noprompt, packagemanager }) => {
        const { migrateCommand } = await Promise.resolve().then(() => tslib_1.__importStar(require('./tasks/migrate')));
        await migrateCommand(config, noprompt, packagemanager);
    }));
    commander_1.program.arguments('[command]').action((0, cli_1.wrapAction)(async (cmd) => {
        if (typeof cmd === 'undefined') {
            log_1.output.write(`\n  ${(0, emoji_1.emoji)('⚡️', '--')}  ${colors_1.default.strong('Capacitor - Cross-Platform apps with JavaScript and the Web')}  ${(0, emoji_1.emoji)('⚡️', '--')}\n\n`);
            commander_1.program.outputHelp();
        }
        else {
            (0, errors_1.fatal)(`Unknown command: ${colors_1.default.input(cmd)}`);
        }
    }));
    commander_1.program.parse(process.argv);
}
exports.runProgram = runProgram;
