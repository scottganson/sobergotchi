<!-- Rock, paper, scissors when two friends who already know each other touch again. -->
<screen id="rps">
  <text h="32" font="name-s" align="center" margin="6 0 0 0">{user.name}</text>
  <image h="56" src="rps" index="{rps.mine}"/>
  <text h="26" font="label-b" align="center">VS</text>
  <image h="56" src="rps" index="{rps.theirs}"/>
  <text h="32" font="name-s" align="center">{rps.name}</text>
  <rule h="9" margin="0 20"/>
  <text h="*" font="script" align="center">{rps.result}</text>
</screen>
