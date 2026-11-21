<!-- A list to pick from: Gratitude jar (up to three), Your word (one), Share your story (one line per page).
     Up / down move, enter ticks a row or does the last ones (Done / < Back). -->
<screen id="pick">
  <row h="21" fill="ink" pad="0 4 0 5">
    <text w="*" font="head">{pick.title}</text>
    <text w="50" font="small-b" align="right">{pick.right}</text>
  </row>
  <wrap h="28" font="small" lh="11" margin="3 5 0 5" align="center">{pick.text}</wrap>
  <list h="*" bind="pick" rowh="21" sel="{pick.sel}">
    <row h="20" pad="2 4 1 5" gap="5">
      <stack w="12">
        <col if="item.box" border="1" radius="2" margin="4 0 4 0"/>
        <col if="item.on" fill="ink" radius="2" margin="4 0 4 0"/>
        <text if="item.back" font="label-b" align="center">&lt;</text>
        <text if="item.done" font="label-b" align="center">&gt;</text>
      </stack>
      <text w="*" font="label">{item.label}</text>
    </row>
    <rule h="1" style="dotted" margin="0 4"/>
  </list>
  <text h="12" font="small" align="center">{hint}</text>
</screen>
