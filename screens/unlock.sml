<!-- Pip's celebration when something on Your path opens. Up/down choose, enter acts. -->
<screen id="unlock">
  <text h="30" font="script-s" align="center" valign="bottom">Something new!</text>
  <image h="96" src="gnomebig" index="9" valign="bottom"/>
  <text h="15" font="head" align="center">{unlock.when}</text>
  <rule h="5" style="dotted" margin="0 30"/>
  <text h="16" font="label-b" align="center">{unlock.name}</text>
  <wrap h="*" font="label" lh="13" margin="0 4" align="center">{unlock.text}</wrap>
  <stack h="22" margin="2 8"><text if="unlock.try" font="small-b" align="center" fill="ink" radius="5">Try it now</text><text if="!unlock.try" font="small-b" align="center" border="1" radius="5">Try it now</text></stack>
  <stack h="22" margin="2 8 4 8"><text if="!unlock.try" font="small-b" align="center" fill="ink" radius="5">Later</text><text if="unlock.try" font="small-b" align="center" border="1" radius="5">Later</text></stack>
</screen>
