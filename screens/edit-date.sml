<!-- Date editor. KEY1 minus, KEY3 plus, KEY2 next field (the last one saves). -->
<screen id="edit-date">
  <row h="21" fill="ink" pad="0 4 0 5">
    <text w="*" font="label-b">{edit.title}</text>
  </row>
  <spacer h="10"/>
  <stack h="38" margin="2 6">
    <row if="!edit.s0" border="1" radius="6" pad="0 8"><text w="*" font="small">{edit.l0}</text><text w="50" font="phone" align="right">{edit.v0}</text></row>
    <row if="edit.s0" fill="ink" radius="6" pad="0 8"><text w="*" font="small">{edit.l0}</text><text w="50" font="phone" align="right">{edit.v0}</text></row>
  </stack>
  <stack h="38" margin="2 6">
    <row if="!edit.s1" border="1" radius="6" pad="0 8"><text w="*" font="small">{edit.l1}</text><text w="50" font="phone" align="right">{edit.v1}</text></row>
    <row if="edit.s1" fill="ink" radius="6" pad="0 8"><text w="*" font="small">{edit.l1}</text><text w="50" font="phone" align="right">{edit.v1}</text></row>
  </stack>
  <stack h="38" margin="2 6">
    <row if="!edit.s2" border="1" radius="6" pad="0 8"><text w="*" font="small">{edit.l2}</text><text w="50" font="phone" align="right">{edit.v2}</text></row>
    <row if="edit.s2" fill="ink" radius="6" pad="0 8"><text w="*" font="small">{edit.l2}</text><text w="50" font="phone" align="right">{edit.v2}</text></row>
  </stack>
  <stack h="38" margin="2 6">
    <row if="edit.has3" border="1" radius="6" pad="0 8"><text w="*" font="small">{edit.l3}</text><text w="50" font="phone" align="right">{edit.v3}</text></row>
    <row if="edit.s3" fill="ink" radius="6" pad="0 8"><text w="*" font="small">{edit.l3}</text><text w="50" font="phone" align="right">{edit.v3}</text></row>
  </stack>
  <spacer h="*"/>
  <text h="13" font="small" align="center">◀ less    more ▶</text>
  <text h="13" font="small" align="center">● next    hold ● cancel</text>
  <spacer h="4"/>
</screen>
