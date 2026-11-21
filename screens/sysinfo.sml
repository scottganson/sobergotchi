<!-- System info, opened from Settings. Enter goes back to Settings. -->
<screen id="sysinfo">
  <row h="21" fill="ink" pad="0 4 0 5">
    <text w="*" font="head">System info</text>
  </row>
  <col h="27" pad="3 5 2 6">
    <text h="13" font="label">Battery</text>
    <text h="*" font="small">{sys.battery}</text>
  </col>
  <rule h="1" style="dotted" margin="0 5"/>
  <col h="27" pad="3 5 2 6">
    <text h="13" font="label">Power</text>
    <text h="*" font="small">{sys.power}</text>
  </col>
  <rule h="1" style="dotted" margin="0 5"/>
  <col h="27" pad="3 5 2 6">
    <text h="13" font="label">Version</text>
    <text h="*" font="small">{sys.version}</text>
  </col>
  <rule h="1" style="dotted" margin="0 5"/>
  <col h="27" pad="3 5 2 6">
    <text h="13" font="label">Friends saved</text>
    <text h="*" font="small">{sys.friends}</text>
  </col>
  <rule h="1" style="dotted" margin="0 5"/>
  <col h="27" pad="3 5 2 6">
    <text h="13" font="label">Device ID</text>
    <text h="*" font="small">{sys.id}</text>
  </col>
  <rule h="1" style="dotted" margin="0 5"/>
  <spacer h="*"/>
  <row h="22" margin="0 4 4 4" pad="0 30">
    <text w="*" font="small-b" align="center" fill="ink" radius="5">&lt; Back</text>
  </row>
</screen>
