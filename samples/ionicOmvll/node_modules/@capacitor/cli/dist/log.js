"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.logSuccess = exports.logPrompt = exports.logger = exports.output = void 0;
const tslib_1 = require("tslib");
const cli_framework_output_1 = require("@ionic/cli-framework-output");
const colors_1 = tslib_1.__importDefault(require("./colors"));
const term_1 = require("./util/term");
const options = {
    colors: colors_1.default,
    stream: process.argv.includes('--json') ? process.stderr : process.stdout,
};
exports.output = (0, term_1.isInteractive)()
    ? new cli_framework_output_1.TTYOutputStrategy(options)
    : new cli_framework_output_1.StreamOutputStrategy(options);
exports.logger = (0, cli_framework_output_1.createDefaultLogger)({
    output: exports.output,
    formatterOptions: {
        titleize: false,
        tags: new Map([
            [cli_framework_output_1.LOGGER_LEVELS.DEBUG, colors_1.default.log.DEBUG('[debug]')],
            [cli_framework_output_1.LOGGER_LEVELS.INFO, colors_1.default.log.INFO('[info]')],
            [cli_framework_output_1.LOGGER_LEVELS.WARN, colors_1.default.log.WARN('[warn]')],
            [cli_framework_output_1.LOGGER_LEVELS.ERROR, colors_1.default.log.ERROR('[error]')],
        ]),
    },
});
async function logPrompt(msg, promptObject) {
    const { wordWrap } = await Promise.resolve().then(() => tslib_1.__importStar(require('@ionic/cli-framework-output')));
    const { prompt } = await Promise.resolve().then(() => tslib_1.__importStar(require('prompts')));
    exports.logger.log({
        msg: `${colors_1.default.input('[?]')} ${wordWrap(msg, { indentation: 4 })}`,
        logger: exports.logger,
        format: false,
    });
    return prompt(promptObject, { onCancel: () => process.exit(1) });
}
exports.logPrompt = logPrompt;
function logSuccess(msg) {
    exports.logger.msg(`${colors_1.default.success('[success]')} ${msg}`);
}
exports.logSuccess = logSuccess;
