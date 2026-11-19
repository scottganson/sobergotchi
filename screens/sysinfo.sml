<!-- System info, opened from Settings. Any button goes back to Settings. -->
<screen id="sysinfo">
  <row h="21" fill="ink" pad="0 4 0 5">
    <text w="*" font="label-b">System info</text>
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
  <box h="26" margin="0 10 8 10" fill="ink" radius="6">
    <text h="*" font="label" align="center">Back</text>
  </box>
</screen>
