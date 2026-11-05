# Compatibility policy

MosaicOS prioritizes classic Win32 desktop software first, with separate tracking for x86
and x86-64. Installer support is tracked independently from application runtime support.

A compatibility claim must include:

- exact application name and version;
- executable and installer architecture;
- MosaicOS/ReactOS build identifier and VM configuration;
- install, launch, primary workflow, save/load, and clean-exit results;
- logs or screenshots supporting the result.

Allowed result values: `PASS`, `PARTIAL`, `FAIL`, `NOT_TESTED`. Never infer a PASS from
an executable's PE header or from a successful installer launch.
