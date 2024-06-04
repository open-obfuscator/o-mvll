"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.buildXmlElement = exports.writeXML = exports.parseXML = exports.readXML = void 0;
const tslib_1 = require("tslib");
const utils_fs_1 = require("@ionic/utils-fs");
const xml2js_1 = tslib_1.__importDefault(require("xml2js"));
async function readXML(path) {
    var _a;
    try {
        const xmlStr = await (0, utils_fs_1.readFile)(path, { encoding: 'utf-8' });
        try {
            return await xml2js_1.default.parseStringPromise(xmlStr);
        }
        catch (e) {
            throw `Error parsing: ${path}, ${(_a = e.stack) !== null && _a !== void 0 ? _a : e}`;
        }
    }
    catch (e) {
        throw `Unable to read: ${path}`;
    }
}
exports.readXML = readXML;
function parseXML(xmlStr, options) {
    const parser = options !== undefined
        ? new xml2js_1.default.Parser({ ...options })
        : new xml2js_1.default.Parser();
    let xmlObj;
    parser.parseString(xmlStr, (err, result) => {
        if (!err) {
            xmlObj = result;
        }
    });
    return xmlObj;
}
exports.parseXML = parseXML;
async function writeXML(object) {
    return new Promise(resolve => {
        const builder = new xml2js_1.default.Builder({
            headless: true,
            explicitRoot: false,
            rootName: 'deleteme',
        });
        let xml = builder.buildObject(object);
        xml = xml.replace('<deleteme>', '').replace('</deleteme>', '');
        resolve(xml);
    });
}
exports.writeXML = writeXML;
function buildXmlElement(configElement, rootName) {
    const builder = new xml2js_1.default.Builder({
        headless: true,
        explicitRoot: false,
        rootName: rootName,
    });
    return builder.buildObject(configElement);
}
exports.buildXmlElement = buildXmlElement;
