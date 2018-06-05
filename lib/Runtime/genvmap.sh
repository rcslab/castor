#!/bin/sh

cat Symbol.map | cpp | awk -v vfile=Versions.def -f /usr/share/mk/version_gen.awk > Version.map
