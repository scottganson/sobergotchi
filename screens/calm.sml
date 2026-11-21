<!-- Calm screen: hold down for 2 s on any screen (after setup). The circle grows as you breathe in and shrinks
     as you breathe out, then rests. Enter closes it. Circle: tools/make_breath.py -->
<screen id="calm">
  <row h="21" fill="ink" pad="0 4 0 5">
    <text w="*" font="head">Breathe</text>
  </row>
  <image h="80" src="breath" index="{calm.frame}" margin="3 0 0 0"/>
  <text h="15" font="label" align="center">{calm.say}</text>
  <rule h="7" style="dotted" margin="0 30"/>
  <wrap h="*" font="label" lh="13" margin="1 4 0 4" align="center">God, grant me the serenity to accept the things I cannot change, courage to change the things I can, and wisdom to know the difference.</wrap>
  <row h="22" margin="0 4 4 4" pad="0 30">
    <text w="*" font="small-b" align="center" fill="ink" radius="5">&lt; Back</text>
  </row>
</screen>
