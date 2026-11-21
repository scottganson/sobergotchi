<!-- Pip's note after something is done on Your path (check-in saved, thanks in the jar, a game finished...).
     Closes on enter or after a few seconds. -->
<screen id="pip">
  <text h="34" font="script-s" align="center" valign="bottom">{pop.title}</text>
  <image h="122" src="gnomebig" index="{pop.pose}" valign="bottom"/>
  <rule h="9" style="dotted" margin="0 30"/>
  <wrap h="*" font="label" lh="14" margin="0 5" align="center">{pop.text}</wrap>
  <row h="22" margin="0 4 4 4" pad="0 30">
    <text w="*" font="small-b" align="center" fill="ink" radius="5">OK</text>
  </row>
</screen>
