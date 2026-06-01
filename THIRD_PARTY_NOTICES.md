# Third-Party Notices

Colason is licensed under the **GNU General Public License v3.0** (see [`LICENSE`](./LICENSE)).
It links against and/or bundles the third-party components listed below. Each component
remains under its own license. This file is included in every binary distribution
(DMG / ZIP / Windows archive) to satisfy the attribution and relink obligations of
those licenses — in particular the **LGPLv3** obligations of the Qt framework.

---

## Qt 6 framework — LGPLv3

Colason uses the [Qt 6](https://www.qt.io/) framework. The following Qt modules are
**dynamically linked** (shipped as separate shared libraries / frameworks, never
statically merged into the Colason executable):

| Module | License |
|--------|---------|
| Qt Core / Gui / Widgets | LGPL-3.0-only |
| Qt WebEngineWidgets / WebEngineCore | LGPL-3.0-only (bundles Chromium, see below) |
| Qt WebChannel | LGPL-3.0-only |
| Qt Svg | LGPL-3.0-only |
| Qt PrintSupport | LGPL-3.0-only |
| Qt Concurrent | LGPL-3.0-only |
| Qt Network / OpenGL / Qml / Quick / QuickWidgets / Positioning (WebEngine deps) | LGPL-3.0-only |

**No GPL-only or commercial-only Qt module is used.** All linked Qt modules are
available under the LGPLv3.

### LGPLv3 compliance for redistributors and end users

In accordance with the GNU Lesser General Public License v3.0:

1. **Dynamic linking.** Qt is provided as separate shared libraries. You may replace
   the bundled Qt libraries with a compatible, modified version of Qt and continue to
   use Colason ("relinking right"). On macOS the Qt frameworks live under
   `Colason.app/Contents/Frameworks`; on Windows the Qt DLLs sit next to `colason.exe`.
2. **Source availability.** The complete corresponding source code of the Qt version
   bundled with this release is available from the Qt Project:
   - https://download.qt.io/archive/qt/  (select the matching 6.x version)
   - https://code.qt.io/cgit/qt/qt5.git/  (git mirror)
   If you cannot obtain the matching Qt source through the links above, the copyright
   holder will provide it on written request to **legal@creanest.co**.
3. **License texts.** The full LGPLv3 and GPLv3 texts are included with every
   distribution. The LGPLv3 text is available at
   https://www.gnu.org/licenses/lgpl-3.0.txt and incorporates the GPLv3 by reference.

### Chromium (via Qt WebEngine)

Qt WebEngine embeds the Chromium project, which is distributed under the
BSD-3-Clause license together with a large set of additional third-party licenses.
The complete Chromium license listing for the bundled version is shipped inside the
Qt WebEngine resources (`qtwebengine_devtools_resources` / `LICENSE.chromium.html`)
and is reproduced in the Qt source archive referenced above.

---

## Web editor bundle (npm) — MIT / BSD

The in-app editor is a web bundle rendered inside Qt WebEngine. It is built from the
following open-source packages (license noted; full texts ship in the JS bundle's
`THIRD_PARTY_LICENSES` and in each package's `node_modules` entry):

| Package | License |
|---------|---------|
| @tiptap/* (core, starter-kit, extensions) | MIT |
| @tiptap/pm (ProseMirror) | MIT |
| codemirror, @codemirror/* | MIT |
| @replit/codemirror-vim | MIT |
| katex | MIT |
| marked | MIT |
| mermaid | MIT |
| lowlight | MIT |
| highlight.js | BSD-3-Clause |

---

## Other native dependencies

| Library | License |
|---------|---------|
| nlohmann/json | MIT |
| spdlog | MIT |
| GoogleTest (test builds only, not distributed) | BSD-3-Clause |

---

*For licensing questions or to request corresponding source for any LGPL/GPL component,
contact legal@creanest.co.*
