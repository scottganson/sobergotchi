<!-- Pip's first-time setup, shown after a fresh flash or a factory reset. < Back / Next > buttons: up / down choose, enter presses;
     the buttons page shows where enter, up and down are; the name, sobriety date and phone pages open the usual editors. -->
<screen id="intro">
  <row h="21" fill="ink" pad="0 4 0 5">
    <text w="*" font="head">{intro.title}</text>
    <text w="30" font="small-b" align="right">{intro.step}</text>
  </row>
  <stack h="96">
    <image if="!intro.buttons" src="gnomebig" index="{intro.pose}" valign="bottom"/>
    <image if="intro.buttons" src="buttons"/>
  </stack>
  <wrap h="*" font="label" lh="13" margin="3 4 0 4" align="center">{intro.text}</wrap>
  <stack h="22" margin="2 4 4 4">
    <row if="intro.hasback" gap="6">
      <stack w="*">
        <text if="intro.bsel" font="small-b" align="center" fill="ink" radius="5">&lt; Back</text>
        <text if="!intro.bsel" font="small-b" align="center" border="1" radius="5">&lt; Back</text>
      </stack>
      <stack w="*">
        <text if="!intro.bsel" font="small-b" align="center" fill="ink" radius="5">{intro.next}</text>
        <text if="intro.bsel" font="small-b" align="center" border="1" radius="5">{intro.next}</text>
      </stack>
    </row>
    <row if="!intro.hasback" pad="0 30">
      <text w="*" font="small-b" align="center" fill="ink" radius="5">{intro.next}</text>
    </row>
  </stack>
</screen>
