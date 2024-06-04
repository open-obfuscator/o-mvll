import type { CapacitorGlobal } from './definitions';
import type { CapacitorInstance, WindowCapacitor } from './definitions-internal';
export interface RegisteredPlugin {
    readonly name: string;
    readonly proxy: any;
    readonly platforms: ReadonlySet<string>;
}
export declare const createCapacitor: (win: WindowCapacitor) => CapacitorInstance;
export declare const initCapacitorGlobal: (win: any) => CapacitorGlobal;
