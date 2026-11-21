<!-- Leaf catch (opens 30 days after setup): leaves fall down three lanes; up / down move the basket a lane left /
     right, enter stops. One step every LEAF_STEP_MS. -->
<screen id="leaf">
  <row h="21" fill="ink" pad="0 4 0 5">
    <text w="*" font="head">Leaf catch</text>
    <text w="50" font="small-b" align="right">{lf.score}</text>
  </row>
  <spacer h="4"/>
  <row h="30"><stack w="*"><image if="lf.c00" src="leaf"/></stack><stack w="*"><image if="lf.c01" src="leaf"/></stack><stack w="*"><image if="lf.c02" src="leaf"/></stack></row>
  <row h="30"><stack w="*"><image if="lf.c10" src="leaf"/></stack><stack w="*"><image if="lf.c11" src="leaf"/></stack><stack w="*"><image if="lf.c12" src="leaf"/></stack></row>
  <row h="30"><stack w="*"><image if="lf.c20" src="leaf"/></stack><stack w="*"><image if="lf.c21" src="leaf"/></stack><stack w="*"><image if="lf.c22" src="leaf"/></stack></row>
  <row h="30"><stack w="*"><image if="lf.c30" src="leaf"/></stack><stack w="*"><image if="lf.c31" src="leaf"/></stack><stack w="*"><image if="lf.c32" src="leaf"/></stack></row>
  <row h="30"><stack w="*"><image if="lf.c40" src="leaf"/></stack><stack w="*"><image if="lf.c41" src="leaf"/></stack><stack w="*"><image if="lf.c42" src="leaf"/></stack></row>
  <row h="22"><stack w="*"><image if="lf.b0" src="basket" valign="bottom"/></stack><stack w="*"><image if="lf.b1" src="basket" valign="bottom"/></stack><stack w="*"><image if="lf.b2" src="basket" valign="bottom"/></stack></row>
  <rule h="3" margin="0 2"/>
  <text h="14" font="small" align="center">{lf.say}</text>
  <text h="*" font="small" align="center">{hint}</text>
</screen>
