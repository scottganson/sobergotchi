<!-- Asks before the sobriety date is changed or a factory reset. KEY1 / KEY3 choose, KEY2 ok. Starts on Cancel. -->
<screen id="confirm">
  <row h="21" fill="ink" pad="0 4 0 5">
    <text w="*" font="head">{confirm.title}</text>
  </row>
  <spacer h="22"/>
  <wrap h="70" font="label-b" lh="15" margin="0 6" align="center">{confirm.text}</wrap>
  <spacer h="*"/>
  <stack h="22" margin="3 8"><text if="confirm.yes" font="small-b" align="center" fill="ink" radius="5">{confirm.ok}</text><text if="!confirm.yes" font="small-b" align="center" border="1" radius="5">{confirm.ok}</text></stack>
  <stack h="22" margin="3 8"><text if="!confirm.yes" font="small-b" align="center" fill="ink" radius="5">Cancel</text><text if="confirm.yes" font="small-b" align="center" border="1" radius="5">Cancel</text></stack>
  <spacer h="*"/>
  <text h="13" font="small" align="center">{hint}</text>
  <spacer h="4"/>
</screen>
