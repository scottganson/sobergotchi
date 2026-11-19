<!-- Asks before the sobriety date is changed. KEY1 / KEY3 choose, KEY2 ok. Starts on Cancel. -->
<screen id="confirm">
  <row h="21" fill="ink" pad="0 4 0 5">
    <text w="*" font="label-b">Sobriety date</text>
  </row>
  <spacer h="22"/>
  <wrap h="70" font="label-b" lh="15" margin="0 6" align="center">Are you sure you want to change your sobriety date?</wrap>
  <spacer h="*"/>
  <stack h="38" margin="2 10">
    <row if="!confirm.yes" border="1" radius="6"><text w="*" font="label" align="center">Yes, change it</text></row>
    <row if="confirm.yes" fill="ink" radius="6"><text w="*" font="label" align="center">Yes, change it</text></row>
  </stack>
  <stack h="38" margin="2 10">
    <row if="confirm.yes" border="1" radius="6"><text w="*" font="label" align="center">Cancel</text></row>
    <row if="!confirm.yes" fill="ink" radius="6"><text w="*" font="label" align="center">Cancel</text></row>
  </stack>
  <spacer h="*"/>
  <text h="13" font="small" align="center">{hint}</text>
  <spacer h="4"/>
</screen>
