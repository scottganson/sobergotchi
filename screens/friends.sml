<!-- Friends, soonest anniversary first, then a "Back" row. KEY2 to pick a friend, KEY2 again to open the card. -->
<screen id="friends">
  <row h="21" fill="ink" pad="0 4 0 5">
    <text w="*" font="label-b">Friends</text>
    <text w="30" font="small-b" align="right">{friends.count}</text>
  </row>
  <stack h="*">
    <list if="friends.any" bind="friends" rowh="32" sel="{friends.sel}">
      <row h="31" pad="3 4 2 4" gap="5">
        <col if="!item.back" w="27" border="1" radius="4">
          <text h="11" font="small-b" align="center" fill="ink" radius="3">{item.date:MMM}</text>
          <text h="*" font="label-b" align="center">{item.date:d}</text>
        </col>
        <col w="*">
          <text h="14" font="label">{item.name}</text>
          <text if="!item.back" h="*" font="small">{item.years} yrs · {item.in}</text>
        </col>
      </row>
      <rule h="1" style="dotted" margin="0 4"/>
    </list>
    <col if="!friends.any">
      <image h="122" src="addfriend"/>
      <text h="18" font="label-b" align="center">No friends yet</text>
      <text h="13" font="small" align="center">Hold the backs of two</text>
      <text h="13" font="small" align="center">Sobergotchis together</text>
    </col>
  </stack>
  <text h="12" font="small" align="center">{hint}</text>
  <dots h="9" count="5"/>
</screen>
