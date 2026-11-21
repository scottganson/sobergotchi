<!-- Tree tiles (opens 6 months after setup): a 3x3 sliding puzzle. Up / down choose a tile next to the gap (or
     < Back), enter slides it. -->
<screen id="tiles">
  <row h="21" fill="ink" pad="0 4 0 5">
    <text w="*" font="head">Tree tiles</text>
    <text w="54" font="small-b" align="right">{tl.moves}</text>
  </row>
  <wrap h="34" font="small" lh="11" margin="6 5 0 5" align="center">Slide the pieces to put the tree back together.</wrap>
  <row h="38" pad="0 4"><stack w="38"><image src="tiles" index="{tl.v0}"/><col if="tl.s0" border="3"/></stack><stack w="38"><image src="tiles" index="{tl.v1}"/><col if="tl.s1" border="3"/></stack><stack w="38"><image src="tiles" index="{tl.v2}"/><col if="tl.s2" border="3"/></stack></row>
  <row h="38" pad="0 4"><stack w="38"><image src="tiles" index="{tl.v3}"/><col if="tl.s3" border="3"/></stack><stack w="38"><image src="tiles" index="{tl.v4}"/><col if="tl.s4" border="3"/></stack><stack w="38"><image src="tiles" index="{tl.v5}"/><col if="tl.s5" border="3"/></stack></row>
  <row h="38" pad="0 4"><stack w="38"><image src="tiles" index="{tl.v6}"/><col if="tl.s6" border="3"/></stack><stack w="38"><image src="tiles" index="{tl.v7}"/><col if="tl.s7" border="3"/></stack><stack w="38"><image src="tiles" index="{tl.v8}"/><col if="tl.s8" border="3"/></stack></row>
  <spacer h="*"/>
  <stack h="22" margin="3 30 3 30"><text if="tl.back" font="small-b" align="center" fill="ink" radius="5">&lt; Back</text><text if="!tl.back" font="small-b" align="center" border="1" radius="5">&lt; Back</text></stack>
  <text h="12" font="small" align="center">{hint}</text>
  <spacer h="3"/>
</screen>
