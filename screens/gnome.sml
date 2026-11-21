<!-- The gnome's page: what he is up to today, and his note when he has one. -->
<screen id="gnome">
  <row h="21" fill="ink" pad="0 4 0 5">
    <text w="*" font="head">Pip</text>
    <text w="42" font="small-b" align="right">{today:MMM d}</text>
  </row>
  <image h="126" src="gnomebig" index="{gnome.pose}" valign="bottom"/>
  <stack h="18">
    <text if="note.any" font="head" align="center">{note.short}</text>
  </stack>
  <wrap h="*" font="label" lh="13" margin="2 4 0 4" align="center">{note.full}</wrap>
  <dots h="9" count="8"/>
</screen>
