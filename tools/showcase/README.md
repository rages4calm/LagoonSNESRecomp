# Local recording scene picker

Developer helper for manually recording Lagoon footage. Build from the workspace
root with `dotnet publish LagoonSNESRecomp/tools/showcase/LagoonShowcase.csproj
-c Release --self-contained false -o Showcase` (one line).

Requires .NET8 Desktop, the installed qualified game, local clean ROM, configuration,
mod catalog and four private validation fixture states. These inputs are not
included in the public repository. The executable and fixture digests are guarded.
It reuses one isolated recording session; owner progress is not copied.
Diagnostic achievements are disabled. Scenes load, then return physical control
to the player. Recording, visual review and actual controller testing are manual.
