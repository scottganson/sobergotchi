<!-- Name / phone editor. KEY1/KEY3 (up/down) change the letter, KEY2 (enter) next letter; enter on a blank opens Save / Cancel / Space. -->
<screen id="edit-text">
  <row h="21" fill="ink" pad="0 4 0 5">
    <text w="*" font="label-b">{edit.title}</text>
  </row>
  <spacer h="16"/>
  <text h="22" font="label-b" align="center" margin="0 4">{edit.text}</text>
  <rule h="1" margin="0 10"/>
  <spacer h="22"/>
  <row h="56" pad="0 8" gap="6">
    <text w="*" font="phone" align="center">{edit.prev}</text>
    <text w="44" font="phone" align="center" fill="ink" radius="8">{edit.cur}</text>
    <text w="*" font="phone" align="center">{edit.next}</text>
  </row>
  <spacer h="*"/>
  <text h="13" font="small" align="center">{edit.hint1}</text>
  <text h="13" font="small" align="center">{edit.hint2}</text>
  <text h="13" font="small" align="center">{edit.hint3}</text>
  <spacer h="4"/>
</screen>
