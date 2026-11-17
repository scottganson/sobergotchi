# Screen markup (SML)

Each file in `screens/` describes one screen on the 122 x 250 portrait display. `tools/build_ui.py` turns them into tables the firmware draws, and renders a preview PNG of each one into `preview/`.

```html
<screen id="main">
  <row h="15" pad="3 3 0 4">
    <text w="*" font="small-b">{today:dddd, MMM d}</text>
    <battery w="28" align="right"/>
  </row>
  <text h="27" font="name" align="center">{user.name}</text>
  <stack h="*"> sky, treemask (draw="paper") and tree images </stack>
</screen>
```

## Layout

| Element | What it does |
| --- | --- |
| `<col>` / `<box>` | stacks children top to bottom (`<screen>` is a col) |
| `<row>` | places children left to right |
| `<stack>` | puts children on top of each other; use with `if` to swap content |
| `<spacer>` | empty space |

Sizes: in a col, give children `h="20"` (pixels), `h="*"` or `h="2*"` (a share of what is left), or leave it out for the natural size. In a row the same goes for `w`. Children always fill the other direction.

Spacing: `pad="t r b l"` inside a container, `margin="t r b l"` around any element, `gap="4"` between a container's children. One, two or four numbers, like CSS.

Decoration on any element: `fill="ink"` (black; content inside turns white), `fill="paper"`, `fill="g12|g25|g50|g75"` (dither textures), `border="1"`, `radius="6"`.

Show only sometimes: `if="next.any"` or `if="!next.any"`.

## Content

| Element | Attributes |
| --- | --- |
| `<text>` | `font`, `align` (left, center, right), `valign` (top, middle, bottom). Too-long text ends in "..". |
| `<wrap>` | a word-wrapped paragraph: `font`, `lh` (line height), `page="{reflect.page}"` |
| `<image>` | `src` (file name in `art/icons` or `art/legacy`, or a group: `tree`, `rps`), `index` for groups, `align`, `valign` |
| `<battery>` | battery icon with a charging bolt |
| `<dots>` | page dots: `count`, `index` (defaults to the current screen) |
| `<rule>` | a line: `style="dotted"`, `dir="v"` for vertical |
| `<list>` | repeats its children per item: `bind="friends"` or `"menu"`, `rowh`, `sel="{friends.sel}"` (that row is drawn inverted) |

Text can contain `{key}` or `{key:format}`. Dates take formats built from `dddd ddd MMMM MMM MM M dd d yyyy yy` (for example `{today:ddd, MMM d}` gives "Sun, Nov 15"). Numbers take `,` for thousands. The characters `·` and `—` and `●` `▲` `▼` are available in every font that has them.

## Fonts

Named in `fonts/fonts.json`: `small`, `small-b`, `label`, `label-b`, `body`, `title`, `name`, `name-s`, `phone`, `big` (digits), `huge` (digits), `script`, `script-s`. Add a TTF/OTF to `fonts/` and an entry to the JSON to make a new one.

## Values the firmware provides

| Key | Example |
| --- | --- |
| `today` (date) | Nov 15, 2026 |
| `user.name`, `user.phone` | Scott, 416-555-0134 |
| `sober.date` (date), `sober.days`, `sober.short` | Aug 19, 2024 · 818 · 2y 2m 27d |
| `sober.num`, `sober.unit` | 2 · YEARS (on celebration days) |
| `tree.stage` | 0 to 25 |
| `sky.index` | 0 to 12, the sky for today (changes daily) |
| `battery.pct`, `battery.charging` | 82 · (empty) |
| `screen.index`, `hint` | 0 · "▲ ▼ move · ● open" |
| `next.any`, `next.today`, `next.name`, `next.years`, `next.date`, `next.in` | the friend with the soonest anniversary |
| `friends.any`, `friends.count`, `friends.sel` | |
| list `friends`: `item.name`, `item.date`, `item.years`, `item.in`, `item.phone`, `item.today` | sorted by next anniversary |
| list `menu`: `item.label`, `item.value` | |
| `reflect.title`, `reflect.text`, `reflect.page`, `reflect.more` | "1/2" when there are more pages |
| `friend.name`, `friend.date`, `friend.years`, `friend.phone`, `friend.in`, `friend.confirm` | friend card and "Friend added" |
| `rps.name`, `rps.mine`, `rps.theirs`, `rps.result` | rock-paper-scissors |
| `edit.title`, `edit.l0..3`, `edit.v0..3`, `edit.s0..3`, `edit.has3`, `edit.text`, `edit.prev`, `edit.cur`, `edit.next` | editors |

Sample values for the previews live in `tools/preview_data.json`. To add a new value, add it to `Data::value()` in `firmware/sobergotchi/src/app.cpp` and to the preview data.

## Art

SVG files are painted in order, so later shapes cover earlier ones. Black becomes solid ink, white becomes paper, and greys become dither textures (light grey = sparse dots, mid grey = checkerboard). PNG files are converted at a 50% threshold. Files in `art/tree/` and `art/rps/` form numbered groups in file-name order.
