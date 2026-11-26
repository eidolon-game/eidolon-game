#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT
cd "$root"
"${CXX:-c++}" --version
git rev-parse HEAD
git hash-object Source/Eidolon/AI/EidolonAIPolicy.h tests/npc_ai_policy.cpp Source/Eidolon/AI/EidolonM2Policy.h Source/Eidolon/AI/EidolonM2Adapters.h Source/Eidolon/AI/EidolonM2PolicyTests.cpp.txt
"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror -pedantic tests/npc_ai_policy.cpp -o "$tmp/legacy"
"$tmp/legacy"
echo 'Legacy PASS'
"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror -pedantic -x c++ Source/Eidolon/AI/EidolonM2PolicyTests.cpp.txt -o "$tmp/m2"
"$tmp/m2"
echo 'M2 PASS'
