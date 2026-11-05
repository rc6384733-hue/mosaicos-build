# MosaicOS component policy

MosaicOS follows a pragmatic, Windows-first, independence-progressive strategy.

1. **Compatibility is the product requirement.** The primary runtime remains the ReactOS NT/Win32 line until another implementation passes the same application and driver test suite.
2. **Reuse is preferred over rewriting.** External components may be used when technically compatible, maintained, auditable, and license-compatible.
3. **No kernel mash-up.** Linux/BSD userland tools and libraries may be integrated where interfaces permit. Linux kernel drivers are not assumed to load in an NT kernel; they require a separately engineered compatibility layer or a native port.
4. **Replace only with evidence.** A Mosaic implementation replaces an upstream component only after passing the same regression suite and showing a measurable benefit.
5. **No leaked/proprietary source.** Use clean-room implementations, public documentation, published specifications, and legally obtained source. Do not import leaked Windows source or confidential material.
6. **Record provenance.** Every external component needs upstream URL, exact version/commit, license, local patches, security status, and test coverage before it is accepted.

## Candidate component register

`COMPONENTS.csv` is a planning register, not a claim that these projects are already integrated. Status `candidate` means evaluated for future use only.
