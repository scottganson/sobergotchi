<!-- Asks before the sobriety date is changed or a factory reset. KEY1 / KEY3 choose, KEY2 ok. Starts on Cancel. -->
<screen id="confirm">
  <row h="21" fill="ink" pad="0 4 0 5">
    <text w="*" font="label-b">{confirm.title}</text>
  </row>
  <spacer h="22"/>
  <wrap h="70" font="label-b" lh="15" margin="0 6" align="center">{confirm.text}</wrap>
  <spacer h="*"/>
  <stack h="38" margin="2 10">
    <row if="!confirm.yes" border="1" radius="6"><text w="*" font="label" align="center">{confirm.ok}</text></row>
    <row if="confirm.yes" fill="ink" radius="6"><text w="*" font="label" align="center">{confirm.ok}</text></row>
  </stack>
  <stack h="38" margin="2 10">
    <row if="confirm.yes" border="1" radius="6"><text w="*" font="label" align="center">Cancel</text></row>
    <row if="!confirm.yes" fill="ink" radius="6"><text w="*" font="label" align="center">Cancel</text></row>
  </stack>
  <spacer h="*"/>
  <text h="13" font="small" align="center">{hint}</text>
  <spacer h="4"/>
</screen>
