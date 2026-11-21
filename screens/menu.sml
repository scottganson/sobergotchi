<!-- Settings. KEY2 to pick an item, KEY2 again to change it. -->
<screen id="menu">
  <row h="21" fill="ink" pad="0 4 0 5">
    <text w="*" font="head">{menu.title}</text>
  </row>
  <list h="*" bind="menu" rowh="27" sel="{menu.sel}" margin="2 0 0 0">
    <col h="26" pad="2 5 2 6">
      <text h="13" font="label">{item.label}</text>
      <text h="*" font="small">{item.value}</text>
    </col>
    <rule h="1" style="dotted" margin="0 5"/>
  </list>
  <text h="12" font="small" align="center">{hint}</text>
  <dots h="9" count="6"/>
</screen>
