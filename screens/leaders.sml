<!-- Leaderboard (carousel page 7): points this month, you and your friends. A friend's score is the one they had
     when you last met. Enter on the page picks a row to scroll; enter again goes back. -->
<screen id="leaders">
  <row h="21" fill="ink" pad="0 4 0 5">
    <text w="*" font="head">Leaderboard</text>
    <text w="34" font="small-b" align="right">{today:MMM}</text>
  </row>
  <list h="*" bind="lb" rowh="27" sel="{lb.sel}">
    <row h="26" pad="3 4 2 4" gap="4">
      <stack w="18">
        <text if="item.top" font="label-b" align="center" fill="ink" radius="9">{item.rank}</text>
        <text if="item.rest" font="label-b" align="center">{item.rank}</text>
        <text if="item.back" font="label-b" align="center">&lt;</text>
      </stack>
      <col w="*">
        <text h="13" font="label">{item.name}</text>
        <text h="*" font="small">{item.seen}</text>
      </col>
      <text w="30" font="label-b" align="right">{item.pts}</text>
    </row>
    <rule h="1" style="dotted" margin="0 4"/>
  </list>
  <text h="12" font="small" align="center">{hint}</text>
  <dots h="9" count="8"/>
</screen>
