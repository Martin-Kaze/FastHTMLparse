#!/usr/bin/env bash
# Linux/macOS version of bench.ps1:
#   ./bench/bench.sh big.html div some-id some-class ./build/fasthtml_bench > new.txt
set -euo pipefail
F=$1; TAG=$2; ID=$3; CLS=$4; EXE=${5:-./build/fasthtml_bench}
[[ -f "$F" ]] || { echo "file not found: $F" >&2; exit 1; }
[[ -x "$EXE" ]] || { echo "not executable: $EXE" >&2; exit 1; }

SEL=("$TAG" "$TAG#$ID" "$TAG.$CLS" "$TAG#$ID.$CLS"
     "div > $TAG" "div > $TAG#$ID" "div > $TAG.$CLS" "div > $TAG#$ID.$CLS")
for i in "${!SEL[@]}"; do
  s=${SEL[$i]}
  out=$("$EXE" "$F" "$s" cb)
  sc=$(awk '/^selectors:/{print $2}' <<<"$out")
  cc=$(awk '/^callbacks:/{print $2}' <<<"$out")
  best=0
  for r in 1 2 3; do
    v=$("$EXE" "$F" "$s" | awk '/^speed:/{printf "%d", $2}')
    (( v > best )) && best=$v
  done
  printf "%d  %-34s sel=%s cb=%s  %d MB/s\n" "$i" "$s" "$sc" "$cc" "$best"
done
