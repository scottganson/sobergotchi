<!-- Pip's first-time setup, shown after a fresh flash or a factory reset. KEY1 (up) back a page, KEY2 (enter) or
     KEY3 (down) next; the name, sobriety date and phone pages open the usual editors. -->
<screen id="intro">
  <row h="21" fill="ink" pad="0 4 0 5">
    <text w="*" font="label-b">{intro.title}</text>
    <text w="30" font="small-b" align="right">{intro.step}</text>
  </row>
  <image h="96" src="gnomebig" index="{intro.pose}" valign="bottom"/>
  <wrap h="*" font="label" lh="13" margin="3 4 0 4" align="center">{intro.text}</wrap>
  <text h="13" font="small" align="center">{hint}</text>
  <spacer h="4"/>
</screen>
