<!-- Shown instead of the home screen on your sobriety anniversary (and 30/60/90 days, 6 and 9 months). -->
<screen id="anniversary">
  <text h="33" font="script" align="center" valign="bottom">Happy</text>
  <text h="30" font="script" align="center">Birthday</text>
  <stack h="106">
    <image src="medallion"/>
    <col pad="24 0 0 0">
      <text h="44" font="huge" align="center">{sober.num}</text>
      <text h="12" font="small-b" align="center">{sober.unit}</text>
    </col>
  </stack>
  <text h="30" font="name" align="center">{user.name}</text>
  <image h="10" src="sprig"/>
  <text h="13" font="small" align="center">sober since {sober.date:MMM d, yyyy}</text>
  <text h="*" font="small-b" align="center">One day at a time</text>
  <dots h="9" count="5"/>
</screen>
