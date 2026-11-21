<!-- Pip asks what to engrave on a newly earned coin. Up / down choose, enter acts. -->
<screen id="engrave">
  <text h="30" font="script-s" align="center" valign="bottom">{eg.title}</text>
  <stack h="56"><image src="coin"/><text font="head" align="center">{eg.num}</text></stack>
  <text h="18" font="head" align="center">{eg.sub}</text>
  <wrap h="*" font="label" lh="13" margin="4 5 0 5" align="center">{eg.text}</wrap>
  <stack h="22" margin="2 8"><text if="!eg.later" font="small-b" align="center" fill="ink" radius="5">Engrave it</text><text if="eg.later" font="small-b" align="center" border="1" radius="5">Engrave it</text></stack>
  <stack h="22" margin="2 8 4 8"><text if="eg.later" font="small-b" align="center" fill="ink" radius="5">Later</text><text if="!eg.later" font="small-b" align="center" border="1" radius="5">Later</text></stack>
</screen>
