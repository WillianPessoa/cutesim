# qmllint baseline — K1 (2026-07-07)

Snapshot of the `all_qmllint` output right after adopting qmlformat, with
qmllint 6.11.0 and the default `.qmllint.ini` (repo root). This is the
reference the K tasks are measured against: **no task may add warnings on
top of this baseline**, and K2/K10/K11 are expected to drive it to zero.

Run it locally with:

```
cmake --build build --target all_qmllint
```

## Totals

| Category | Count | Fixed by |
|---|---|---|
| `unqualified` | 237 | K2 (context properties) + K10 (delegates) |
| `Quick.layout-positioning` | 38 | K9 (implicit sizes sweep) |
| `missing-property` | 0 | was 4 — real bug, fixed as BUG-25 |
| **Total** | **275** | |

Root-cause split of the `unqualified` group (approximate — some warnings
mention both):

- ~2/3 are reads of the untyped context properties `controller` /
  `scenarioBridge` (Main.qml alone has 82). These disappear when K2 turns
  `SimController`/`ScenarioBridge` into `QML_ELEMENT`/`QML_SINGLETON` typed
  access.
- ~1/3 are implicit `modelData` / `index` in delegates. K10 adds
  `pragma ComponentBehavior: Bound` + `required property` per file.

## Per file × category

| File | unqualified | layout-positioning |
|---|---|---|
| Main.qml | 82 | — |
| TimelineChart.qml | 39 | — |
| ScenarioEditor.qml | 33 | 11 |
| ProcessDetail.qml | 20 | 8 |
| FinishedTable.qml | 17 | 1 |
| SegmentControl.qml | 10 | — |
| QueueView.qml | 9 | 2 |
| LaunchOverlay.qml | 9 | 4 |
| CPUView.qml | 8 | 1 |
| InspectorPanel.qml | 5 | 6 |
| Logo.qml | 5 | — |
| StatsBar.qml | — | 3 |
| Header.qml | — | 2 |

## Notes from the baseline run

- The 4 `missing-property` warnings were a real defect: `containsMouse`
  read from `HoverHandler` ids (the property only exists on `MouseArea`;
  `HoverHandler` has `hovered`), so those hover highlights never lit.
  Registered and fixed as **BUG-25** — first bug caught by the tooling.
- qmlformat crashes ("Could not construct the JS DOM") on function
  declarations nested inside a binding block. `InspectorPanel.configText`
  had two; they were hoisted to component-level helpers. Avoid the
  construct going forward.
- `Quick.layout-positioning` = plain `width`/`height` set on items managed
  by a `RowLayout`/`ColumnLayout`; the fix is `Layout.preferredWidth/Height`
  or implicit sizes, which is exactly the K9 sweep.
