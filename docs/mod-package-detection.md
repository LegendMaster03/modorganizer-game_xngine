# Mod package detection, installation roots, and content labels

This describes the current Mod Organizer 2 package inspection code.
Recognition, installation policy, selecting the content root, and labeling
installed content are separate operations.

## Shared behavior

`XngineModDataChecker::dataLooksValid` checks only immediate children of
the supplied file tree for common directories and file extensions. A matching
indicator returns `VALID`; this does not prove format compatibility or
that the game can use the files.

`XngineModDataContent` reports categories for MO2 content icons. Its base
implementation also scans immediate children. These labels are **not**
installation or security gates, and do not invoke format conversion.

## Game-specific decisions

- **Daggerfall:** Recognizes classic data, patch formats, save packs, quest
  source, installer payloads, nested archives, disc images, and Daggerfall
  Unity markers. Plugin settings independently gate executable/JSON/binary
  patching, save-pack installation, and Unity-format compatibility.
  Recognizing an installer does not execute it; recognizing `*.src` does
  not compile QBN/QRC; recognizing nested archives does not extract them.
  Classic Daggerfall can not execute Unity runtime `*.dfmod` behavior.
- **Arena and Battlespire:** Detect directly usable files first and search
  nested distribution wrappers when necessary. `FIXABLE` indicates that
  selecting a subtree may make the package installable. Preferred variant
  ordering uses the detected game installation layout, not a guaranteed
  semantic equivalence between Steam and GOG packages.
- **Redguard:** A root-level `About.txt`, `INI Changes.txt`,
  `Map Changes.txt`, or `RTX Changes.txt` individually triggers the
  Dillon241 patch-package branch, subject to the corresponding setting.
  The previous source comment incorrectly described a required
  `About.txt` plus changes-file combination. Executable payload gating
  is separate. Config filenames from the game's declared settings and
  Redguard data folders provide other recognition paths.

## Nested root selection

Daggerfall scores candidate subtrees using payload indicators, classic
directory conventions, depth, and documentation/optional-folder penalties.
When a `DF/DAGGER/ARENA2` path is selected, it may normalize back to the
containing install root to preserve relative file placement. A positive
score is a heuristic, not a compatibility proof. Arena and Battlespire
instead use ordered candidate search and return the first apparently valid
subtree, falling back to an existing wrapper directory if necessary.

Do not conflate `FIXABLE` with binary repair: these operations choose
a package directory; they do not rewrite archive payloads.

## Verification scope

Check policy gating, subtree selection, actual MO2 file mapping, and
in-game behavior independently. Broad indicators such as `README.TXT`,
archive extensions, or installer filenames can produce false positives
without usable game data.

This documentation does not modify the existing detection or installation
rules.
## Content badges are not executable behavior

Arena additionally labels mapper, DOSBox configuration, and batch helper
files as configuration or script content even when a mod has no ordinary
asset files. Battlespire labels archive/executable-replacement payloads
separately from DOSBox configuration and text. Redguard labels any of
the root-level Dillon241 patch markers as Patch Instructions. These are
MO2 presentation categories, not permissions to execute scripts,
install unsupported archives, or apply patches.
