<!-- One friend's card. Up / down choose Back or Delete, enter does it; Delete asks first (confirm screen). -->
<screen id="friend">
  <row h="21" fill="ink" pad="0 4 0 5">
    <text w="*" font="label-b">Friend</text>
  </row>
  <text h="36" font="name" align="center" margin="6 2 0 2">{friend.name}</text>
  <text h="16" font="label-b" align="center">{friend.years} years sober</text>
  <text h="13" font="small" align="center">since {friend.date:MMM d, yyyy}</text>
  <image h="16" src="sprig"/>
  <image h="16" src="phone"/>
  <text h="20" font="label-b" align="center">{friend.phone}</text>
  <spacer h="8"/>
  <box h="40" margin="0 6" border="1" radius="6" pad="3 2">
    <text h="15" font="small" align="center">Anniversary</text>
    <text h="*" font="small-b" align="center">{friend.date:MMM d} · {friend.in}</text>
  </box>
  <spacer h="*"/>
  <stack h="24" margin="0 4 4 4">
    <row if="!friend.del" gap="6">
      <text w="*" font="small-b" align="center" fill="ink" radius="5">Back</text>
      <text w="*" font="small-b" align="center" border="1" radius="5">Delete</text>
    </row>
    <row if="friend.del" gap="6">
      <text w="*" font="small-b" align="center" border="1" radius="5">Back</text>
      <text w="*" font="small-b" align="center" fill="ink" radius="5">Delete</text>
    </row>
  </stack>
</screen>
