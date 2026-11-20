<!-- Friends: an "Add friend" row, the friends soonest anniversary first, then "Back". With none, enter adds one. -->
<screen id="friends">
  <row h="21" fill="ink" pad="0 4 0 5">
    <text w="*" font="label-b">Friends</text>
    <text w="30" font="small-b" align="right">{friends.count}</text>
  </row>
  <stack h="*">
    <list bind="friends" rowh="32" sel="{friends.sel}">
      <row h="31" pad="3 4 2 4" gap="5">
        <stack w="27">
          <col if="!item.back" border="1" radius="4">
            <text h="11" font="small-b" align="center" fill="ink" radius="3">{item.date:MMM}</text>
            <text h="*" font="label-b" align="center">{item.date:d}</text>
          </col>
          <col if="item.add" border="1" radius="4">
            <text h="*" font="label-b" align="center">+</text>
          </col>
          <col if="item.ret" border="1" radius="4">
            <text h="*" font="label-b" align="center">&lt;</text>
          </col>
        </stack>
        <col w="*">
          <text h="14" font="label">{item.name}</text>
          <text if="!item.back" h="*" font="small">{item.years} yrs · {item.in}</text>
        </col>
      </row>
      <rule h="1" style="dotted" margin="0 4"/>
    </list>
  </stack>
  <text h="12" font="small" align="center">{hint}</text>
  <dots h="9" count="5"/>
</screen>
