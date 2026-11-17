<!-- Date editor. KEY1 (up) plus, KEY3 (down) minus, KEY2 (enter) next field; after the last field a Save / Cancel choice. -->
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
  <text h="13" font="small" align="center">{edit.hint1}</text>
  <text h="13" font="small" align="center">{edit.hint2}</text>
  <spacer h="4"/>
</screen>
