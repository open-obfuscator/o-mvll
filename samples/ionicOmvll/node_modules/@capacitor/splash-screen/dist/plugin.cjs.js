'use strict';

Object.defineProperty(exports, '__esModule', { value: true });

var core = require('@capacitor/core');

const SplashScreen = core.registerPlugin('SplashScreen', {
    web: () => Promise.resolve().then(function () { return web; }).then(m => new m.SplashScreenWeb()),
});

class SplashScreenWeb extends core.WebPlugin {
    async show(_options) {
        return undefined;
    }
    async hide(_options) {
        return undefined;
    }
}

var web = /*#__PURE__*/Object.freeze({
    __proto__: null,
    SplashScreenWeb: SplashScreenWeb
});

exports.SplashScreen = SplashScreen;
//# sourceMappingURL=plugin.cjs.js.map
