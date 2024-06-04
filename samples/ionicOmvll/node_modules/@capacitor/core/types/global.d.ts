import type { WebPlugin } from './web-plugin';
export declare const Capacitor: import("./definitions").CapacitorGlobal;
export declare const registerPlugin: import("./definitions").RegisterPlugin;
/**
 * @deprecated Provided for backwards compatibility for Capacitor v2 plugins.
 * Capacitor v3 plugins should import the plugin directly. This "Plugins"
 * export is deprecated in v3, and will be removed in v4.
 */
export declare const Plugins: import(".").PluginRegistry;
/**
 * Provided for backwards compatibility. Use the registerPlugin() API
 * instead, and provide the web plugin as the "web" implmenetation.
 * For example
 *
 * export const Example = registerPlugin('Example', {
 *   web: () => import('./web').then(m => new m.Example())
 * })
 *
 * @deprecated Deprecated in v3, will be removed from v4.
 */
export declare const registerWebPlugin: (plugin: WebPlugin) => void;
