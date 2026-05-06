#!/bin/bash
# AetherNet Cppcheck Security Scan Wrapper

echo "Starting Cppcheck Security Scan..."

# --enable=warning,style,performance,portability : Broad check categories
# --inline-suppr : Allow inline suppressions in code
# --suppress=missingIncludeSystem : Skip system headers for speed
# --error-exitcode=1 : Fail the CI if errors are found
# --xml : Generate XML output for CI reporting tools

cppcheck --enable=all \
         --inline-suppr \
         --suppress=missingIncludeSystem \
         --std=c++20 \
         --error-exitcode=1 \
         --xml \
         ./aether-core/src 2> cppcheck_report.xml

if [ $? -eq 0 ]; then
    echo "Cppcheck scan completed successfully with no critical errors."
else
    echo "Cppcheck found vulnerabilities. Review cppcheck_report.xml"
    exit 1
fi
