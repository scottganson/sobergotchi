<!-- Today's reflection. KEY2 turns the page when the text is long. -->
<screen id="reflection">
  <row h="21" fill="ink" pad="0 4 0 5">
    <text w="*" font="label-b">Reflection</text>
    <text w="42" font="small-b" align="right">{today:MMM d}</text>
  </row>
  <image h="19" src="quote" align="left" valign="bottom" margin="0 0 0 6"/>
  <wrap h="*" font="body" lh="16" margin="3 5 0 6" page="{reflect.page}">{reflect.text}</wrap>
  <image h="11" src="sprig"/>
  <text h="15" font="small-b" align="center">{reflect.title}</text>
  <text h="11" font="small" align="center">{reflect.more}</text>
  <dots h="9" count="4"/>
</screen>
