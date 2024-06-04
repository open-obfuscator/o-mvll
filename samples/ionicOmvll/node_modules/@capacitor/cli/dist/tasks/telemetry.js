"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.telemetryCommand = void 0;
const tslib_1 = require("tslib");
const colors_1 = tslib_1.__importDefault(require("../colors"));
const errors_1 = require("../errors");
const log_1 = require("../log");
const sysconfig_1 = require("../sysconfig");
const telemetry_1 = require("../telemetry");
async function telemetryCommand(onOrOff) {
    const sysconfig = await (0, sysconfig_1.readConfig)();
    const enabled = interpretEnabled(onOrOff);
    if (typeof enabled === 'boolean') {
        if (sysconfig.telemetry === enabled) {
            log_1.logger.info(`Telemetry is already ${colors_1.default.strong(enabled ? 'on' : 'off')}`);
        }
        else {
            await (0, sysconfig_1.writeConfig)({ ...sysconfig, telemetry: enabled });
            (0, log_1.logSuccess)(`You have ${colors_1.default.strong(`opted ${enabled ? 'in' : 'out'}`)} ${enabled ? 'for' : 'of'} telemetry on this machine.`);
            if (enabled) {
                log_1.output.write(telemetry_1.THANK_YOU);
            }
        }
    }
    else {
        log_1.logger.info(`Telemetry is ${colors_1.default.strong(sysconfig.telemetry ? 'on' : 'off')}`);
    }
}
exports.telemetryCommand = telemetryCommand;
function interpretEnabled(onOrOff) {
    switch (onOrOff) {
        case 'on':
            return true;
        case 'off':
            return false;
        case undefined:
            return undefined;
    }
    (0, errors_1.fatal)(`Argument must be ${colors_1.default.strong('on')} or ${colors_1.default.strong('off')} (or left unspecified)`);
}
