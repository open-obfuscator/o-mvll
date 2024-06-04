"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.receive = exports.send = void 0;
const tslib_1 = require("tslib");
const utils_fs_1 = require("@ionic/utils-fs");
const utils_subprocess_1 = require("@ionic/utils-subprocess");
const debug_1 = tslib_1.__importDefault(require("debug"));
const https_1 = require("https");
const path_1 = require("path");
const cli_1 = require("./util/cli");
const debug = (0, debug_1.default)('capacitor:ipc');
/**
 * Send an IPC message to a forked process.
 */
async function send(msg) {
    const dir = cli_1.ENV_PATHS.log;
    await (0, utils_fs_1.mkdirp)(dir);
    const logPath = (0, path_1.resolve)(dir, 'ipc.log');
    debug('Sending %O IPC message to forked process (logs: %O)', msg.type, logPath);
    const fd = await (0, utils_fs_1.open)(logPath, 'a');
    const p = (0, utils_subprocess_1.fork)(process.argv[1], ['📡'], { stdio: ['ignore', fd, fd, 'ipc'] });
    p.send(msg);
    p.disconnect();
    p.unref();
}
exports.send = send;
/**
 * Receive and handle an IPC message.
 *
 * Assume minimal context and keep external dependencies to a minimum.
 */
async function receive(msg) {
    debug('Received %O IPC message', msg.type);
    if (msg.type === 'telemetry') {
        const now = new Date().toISOString();
        const { data } = msg;
        // This request is only made if telemetry is on.
        const req = (0, https_1.request)({
            hostname: 'api.ionicjs.com',
            port: 443,
            path: '/events/metrics',
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
        }, response => {
            debug('Sent %O metric to events service (status: %O)', data.name, response.statusCode);
            if (response.statusCode !== 204) {
                response.on('data', chunk => {
                    debug('Bad response from events service. Request body: %O', chunk.toString());
                });
            }
        });
        const body = {
            metrics: [data],
            sent_at: now,
        };
        req.end(JSON.stringify(body));
    }
}
exports.receive = receive;
