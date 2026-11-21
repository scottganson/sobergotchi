<!-- Home screen: date and battery, your name, the tree, your sober time, next friend anniversary. -->
<screen id="main">
  <row h="15" pad="3 3 0 4">
    <text w="*" font="small-b">{today:ddd, MMM d}</text>
    <battery w="28" align="right"/>
  </row>
  <text h="27" font="script-s" align="center">{user.name}</text>
  <!-- Layers: the day's sky, the tree's outline cleared out of it, the tree, from five years the day's view of the gnome's world, the gnome himself, and who is where. -->
  <stack h="*">
    <image src="sky" index="{sky.index}" valign="bottom"/>
    <image src="treemask" index="{tree.stage}" valign="bottom" draw="paper"/>
    <image src="tree" index="{tree.stage}" valign="bottom"/>
    <!-- From five years: today's view of the gnome's world (wide, middle or close); frame 0 is empty. -->
    <image src="worldmask" index="{world.index}" valign="bottom" draw="paper"/>
    <image src="world" index="{world.index}" valign="bottom"/>
    <image src="gnomemask" index="{gnome.home}" valign="bottom" draw="paper"/>
    <image src="gnome" index="{gnome.home}" valign="bottom"/>
    <!-- From five years: who is where today (frame 0 is empty). -->
    <image src="homemask" index="{home.index}" valign="bottom" draw="paper"/>
    <image src="home" index="{home.index}" valign="bottom"/>
  </stack>
  <box h="52" margin="1 3 0 3" fill="ink" radius="10" pad="4 4 5 4">
    <text h="31" font="big" align="center" valign="top">{sober.days:,}</text>
    <text h="*" font="small-b" align="center">DAYS · {sober.short}</text>
  </box>
  <!-- On days the gnome has a note, its short line takes the place of the "sober since" line. -->
  <stack h="14">
    <text if="!note.any" font="small" align="center">sober since {sober.date:MMM d, yyyy}</text>
    <text if="note.any" font="small-b" align="center">{note.short}</text>
  </stack>
  <stack h="21" margin="0 3 1 3">
    <row if="!next.today" pad="2 2 1 2" gap="5" border="1" radius="5">
      <image w="16" src="cake"/>
      <stack w="*">
        <text font="small-b" if="next.any">{next.name} - {next.date:MMM d}</text>
        <text font="small" if="!next.any">No friends yet</text>
      </stack>
    </row>
    <row if="next.today" fill="ink" radius="5" pad="2 2 1 2" gap="5">
      <image w="16" src="cake"/>
      <text w="*" font="small-b">{next.name} · {next.years} yrs!</text>
    </row>
  </stack>
  <dots h="9" count="6"/>
</screen>
