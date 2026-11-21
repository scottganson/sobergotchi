<!-- A Step prayer from Your path. Your word, when chosen, opens it. Enter turns the page, then goes back. -->
<screen id="prayer">
  <row h="21" fill="ink" pad="0 4 0 5">
    <text w="*" font="head">{prayer.title}</text>
  </row>
  <image h="12" src="sprig" margin="4 0 0 0"/>
  <wrap h="*" font="body" lh="16" margin="3 5 0 6" page="{prayer.page}">{prayer.a}{prayer.b}{prayer.c}{prayer.d}{prayer.e}{prayer.f}</wrap>
  <text h="13" font="small" align="center">{prayer.more}</text>
  <row h="22" margin="0 4 4 4" pad="0 30">
    <text w="*" font="small-b" align="center" fill="ink" radius="5">{prayer.btn}</text>
  </row>
</screen>
