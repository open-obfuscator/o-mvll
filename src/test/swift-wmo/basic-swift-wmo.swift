//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

// REQUIRES: apple_abi && aarch64-registered-target

// RUN: rm -rf %t && mkdir -p %t
// RUN: env OMVLL_CONFIG=%S/Inputs/config_swift_wmo.py swift-frontend \
// RUN:     -wmo -target arm64-apple-ios14.0 -load-pass-plugin=%libOMVLL -Onone -num-threads 8 \
// RUN:     %S/Inputs/foo.swift %S/Inputs/bar.swift %s -emit-ir %EXTRA_SWIFT_FLAGS \
// RUN:     -o %t/bar.ll -o %t/foo.ll -o %t/main.ll
// RUN: FileCheck %s < %t/main.ll

// CHECK: define swiftcc i64 @"$s3foo4mainSiyF"() {{.*}} {
public func main() -> Int { return foo() + bar() }
