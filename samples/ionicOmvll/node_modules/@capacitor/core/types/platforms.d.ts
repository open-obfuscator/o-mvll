import type { PluginImplementations } from './definitions';
import type { PluginHeader } from './definitions-internal';
export interface CapacitorPlatform {
    name: string;
    getPlatform?(): string;
    isPluginAvailable?(pluginName: string): boolean;
    getPluginHeader?(pluginName: string): PluginHeader | undefined;
    registerPlugin?(pluginName: string, jsImplementations: PluginImplementations): any;
    isNativePlatform?(): boolean;
}
export interface CapacitorPlatformsInstance {
    currentPlatform: CapacitorPlatform;
    platforms: Map<string, CapacitorPlatform>;
    addPlatform(name: string, platform: CapacitorPlatform): void;
    setPlatform(name: string): void;
}
/**
 * @deprecated Set `CapacitorCustomPlatform` on the window object prior to runtime executing in the web app instead
 */
export declare const CapacitorPlatforms: CapacitorPlatformsInstance;
/**
 * @deprecated Set `CapacitorCustomPlatform` on the window object prior to runtime executing in the web app instead
 */
export declare const addPlatform: (name: string, platform: CapacitorPlatform) => void;
/**
 * @deprecated Set `CapacitorCustomPlatform` on the window object prior to runtime executing in the web app instead
 */
export declare const setPlatform: (name: string) => void;
