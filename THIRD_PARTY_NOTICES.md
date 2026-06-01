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
   The exact Qt version bundled in each release is recorded in `licenses/QT_VERSION.txt`
   inside the distributed package. If you cannot obtain the matching Qt source through
   the links above, the copyright holder will provide it on written request to
   **legal@creanest.co**.
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
following open-source packages. The full license text of each package is available in
its `node_modules/<pkg>/LICENSE` entry in this repository and at the package's npm /
GitHub page. The required copyright notices are reproduced below.

| Package | License | Copyright |
|---------|---------|-----------|
| @tiptap/* (core, starter-kit, extensions) | MIT | © 2023 Tiptap GmbH |
| @tiptap/pm (ProseMirror) | MIT | © 2015-2024 Marijn Haverbeke and others |
| codemirror, @codemirror/* | MIT | © 2018 Marijn Haverbeke and others |
| @replit/codemirror-vim | MIT | © Replit and contributors |
| katex | MIT | © 2013-2020 Khan Academy and contributors |
| marked | MIT | © 2018+ MarkedJS; © 2011-2018 Christopher Jeffrey |
| mermaid | MIT | © 2014-2024 Knut Sveidqvist |
| lowlight | MIT | © 2016 Titus Wormer |
| highlight.js | BSD-3-Clause | © 2006 Ivan Sagalaev and other contributors |

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
