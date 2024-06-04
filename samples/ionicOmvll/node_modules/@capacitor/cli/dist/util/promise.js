"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.lazy = exports.LazyPromise = exports.allSerial = void 0;
function allSerial(funcs) {
    return funcs.reduce((promise, func) => promise.then(result => func().then(x => result.concat(x))), Promise.resolve([]));
}
exports.allSerial = allSerial;
class LazyPromise extends Promise {
    constructor(executor) {
        super(() => {
            /* ignore */
        });
        this._executor = executor;
    }
    then(onfulfilled, onrejected) {
        this._promise = this._promise || new Promise(this._executor);
        return this._promise.then(onfulfilled, onrejected);
    }
    catch(onrejected) {
        this._promise = this._promise || new Promise(this._executor);
        return this._promise.catch(onrejected);
    }
}
exports.LazyPromise = LazyPromise;
function lazy(fn) {
    return new LazyPromise(async (resolve, reject) => {
        try {
            resolve(await fn());
        }
        catch (e) {
            reject(e);
        }
    });
}
exports.lazy = lazy;
