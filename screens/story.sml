<!-- A story: yours (Change / < Back) or a friend's (< Back). Three lines picked in Share your story. -->
<screen id="story">
  <row h="21" fill="ink" pad="0 4 0 5">
    <text w="*" font="head">{sv.title}</text>
  </row>
  <image h="18" src="quote" align="left" valign="bottom" margin="4 0 0 6"/>
  <wrap h="44" font="body" lh="16" margin="2 5 0 6">{sv.a}</wrap>
  <wrap h="44" font="body" lh="16" margin="0 5 0 6">{sv.b}</wrap>
  <wrap h="44" font="body" lh="16" margin="0 5 0 6">{sv.c}</wrap>
  <spacer h="*"/>
  <image h="12" src="sprig"/>
  <stack h="24" margin="2 4 4 4">
    <row if="sv.own" gap="6">
      <stack w="*"><text if="!sv.back" font="small-b" align="center" fill="ink" radius="5">Change</text><text if="sv.back" font="small-b" align="center" border="1" radius="5">Change</text></stack>
      <stack w="*"><text if="sv.back" font="small-b" align="center" fill="ink" radius="5">&lt; Back</text><text if="!sv.back" font="small-b" align="center" border="1" radius="5">&lt; Back</text></stack>
    </row>
    <row if="!sv.own" pad="0 26">
      <text w="*" font="small-b" align="center" fill="ink" radius="5">&lt; Back</text>
    </row>
  </stack>
</screen>
