<!-- Daily check-in (opens 7 days after setup): one thing to do just for today. Up / down choose, enter picks. -->
<screen id="checkin">
  <row h="21" fill="ink" pad="0 4 0 5">
    <text w="*" font="head">Check-in</text>
    <text w="42" font="small-b" align="right">{today:MMM d}</text>
  </row>
  <text h="20" font="label-b" align="center" margin="4 0 0 0">Just for today, I will</text>
  <text h="13" font="small" align="center">{ci.done}</text>
  <stack h="22" margin="3 6"><text if="ci.s0" font="small-b" align="center" fill="ink" radius="5">{ci.l0}</text><text if="!ci.s0" font="small-b" align="center" border="1" radius="5">{ci.l0}</text></stack>
  <stack h="22" margin="3 6"><text if="ci.s1" font="small-b" align="center" fill="ink" radius="5">{ci.l1}</text><text if="!ci.s1" font="small-b" align="center" border="1" radius="5">{ci.l1}</text></stack>
  <stack h="22" margin="3 6"><text if="ci.s2" font="small-b" align="center" fill="ink" radius="5">{ci.l2}</text><text if="!ci.s2" font="small-b" align="center" border="1" radius="5">{ci.l2}</text></stack>
  <stack h="22" margin="3 6"><text if="ci.s3" font="small-b" align="center" fill="ink" radius="5">{ci.l3}</text><text if="!ci.s3" font="small-b" align="center" border="1" radius="5">{ci.l3}</text></stack>
  <stack h="22" margin="3 6"><text if="ci.s4" font="small-b" align="center" fill="ink" radius="5">{ci.l4}</text><text if="!ci.s4" font="small-b" align="center" border="1" radius="5">{ci.l4}</text></stack>
  <spacer h="*"/>
  <stack h="22" margin="3 6 6 6"><text if="ci.s5" font="small-b" align="center" fill="ink" radius="5">&lt; Back</text><text if="!ci.s5" font="small-b" align="center" border="1" radius="5">&lt; Back</text></stack>
</screen>
