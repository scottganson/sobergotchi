<!-- Your path (carousel page 5): what Pip has to share, opened by days since setup. Locked rows show a padlock and
     when they open. Enter on the page picks a row; up / down move, enter opens (a locked row says when). -->
<screen id="journey">
  <row h="21" fill="ink" pad="0 4 0 5">
    <text w="*" font="head">Your path</text>
    <text w="50" font="small-b" align="right">day {use.days}</text>
  </row>
  <list h="*" bind="path" rowh="29" sel="{path.sel}">
    <row h="28" pad="3 4 2 4" gap="5">
      <stack w="16">
        <image if="item.locked" src="lock"/>
        <image if="item.open" src="unlock"/>
        <text if="item.back" font="label-b" align="center">&lt;</text>
      </stack>
      <col w="*">
        <text h="13" font="label">{item.name}</text>
        <text h="*" font="small">{item.sub}</text>
      </col>
    </row>
    <rule h="1" style="dotted" margin="0 4"/>
  </list>
  <text h="12" font="small" align="center">{hint}</text>
  <dots h="9" count="6"/>
</screen>
