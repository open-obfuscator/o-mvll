#!/usr/bin/env bash

cp o-mvll/src/build_xcode/omvll_unsigned.dylib o-mvll/src/build_xcode/omvll.dylib
export OMVLL_CERTIFICATE_PWD="HbgUWMB5JXvqjpctQN9E6T"
o-mvll/ci/apple-codesign-0.27.0-aarch64-unknown-linux-musl/rcodesign sign --p12-file o-mvll/ci/certificates/SigningCertificate.p12 --p12-password $OMVLL_CERTIFICATE_PWD --code-signature-flags runtime o-mvll/src/build_xcode/omvll.dylib
