<!-- The back of a year coin: the words engraved when it was earned (locked in for good). Enter goes back. -->
<screen id="coin">
  <row h="21" fill="ink" pad="0 4 0 5">
    <text w="*" font="head">{coin.title}</text>
  </row>
  <text h="20" font="small" align="center" valign="bottom">the back of your coin</text>
  <stack h="108">
    <image src="coinback"/>
    <wrap font="head" lh="16" margin="30 24 0 24" align="center">{coin.text}</wrap>
  </stack>
  <image h="14" src="sprig"/>
  <text h="13" font="small" align="center">earned {coin.date}</text>
  <text h="13" font="small-b" align="center">{coin.locked}</text>
  <spacer h="*"/>
  <row h="22" margin="0 4 4 4" pad="0 30">
    <text w="*" font="small-b" align="center" fill="ink" radius="5">&lt; Back</text>
  </row>
</screen>
