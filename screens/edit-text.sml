<!-- Name / phone keyboard. Up/down step through every key and then Space, Delete, Save and Cancel; enter types the
     highlighted key or does the action. -->
<screen id="edit-text">
  <row h="21" fill="ink" pad="0 4 0 5">
    <text w="*" font="label-b">{edit.title}</text>
  </row>
  <spacer h="5"/>
  <text h="22" font="label-b" align="center" margin="0 4">{edit.text}</text>
  <rule h="1" margin="0 10"/>
  <spacer h="5"/>
  <stack h="21" margin="0 4" if="kb.v0">
    <row pad="2 2"><stack w="*"><text if="!kb.h00" font="label-b" align="center">{kb.c00}</text><text if="kb.h00" fill="ink" radius="3" font="label-b" align="center">{kb.c00}</text></stack><stack w="*"><text if="!kb.h01" font="label-b" align="center">{kb.c01}</text><text if="kb.h01" fill="ink" radius="3" font="label-b" align="center">{kb.c01}</text></stack><stack w="*"><text if="!kb.h02" font="label-b" align="center">{kb.c02}</text><text if="kb.h02" fill="ink" radius="3" font="label-b" align="center">{kb.c02}</text></stack><stack w="*"><text if="!kb.h03" font="label-b" align="center">{kb.c03}</text><text if="kb.h03" fill="ink" radius="3" font="label-b" align="center">{kb.c03}</text></stack><stack w="*"><text if="!kb.h04" font="label-b" align="center">{kb.c04}</text><text if="kb.h04" fill="ink" radius="3" font="label-b" align="center">{kb.c04}</text></stack></row>
  </stack>
  <stack h="21" margin="0 4" if="kb.v1">
    <row pad="2 2"><stack w="*"><text if="!kb.h10" font="label-b" align="center">{kb.c10}</text><text if="kb.h10" fill="ink" radius="3" font="label-b" align="center">{kb.c10}</text></stack><stack w="*"><text if="!kb.h11" font="label-b" align="center">{kb.c11}</text><text if="kb.h11" fill="ink" radius="3" font="label-b" align="center">{kb.c11}</text></stack><stack w="*"><text if="!kb.h12" font="label-b" align="center">{kb.c12}</text><text if="kb.h12" fill="ink" radius="3" font="label-b" align="center">{kb.c12}</text></stack><stack w="*"><text if="!kb.h13" font="label-b" align="center">{kb.c13}</text><text if="kb.h13" fill="ink" radius="3" font="label-b" align="center">{kb.c13}</text></stack><stack w="*"><text if="!kb.h14" font="label-b" align="center">{kb.c14}</text><text if="kb.h14" fill="ink" radius="3" font="label-b" align="center">{kb.c14}</text></stack></row>
  </stack>
  <stack h="21" margin="0 4" if="kb.v2">
    <row pad="2 2"><stack w="*"><text if="!kb.h20" font="label-b" align="center">{kb.c20}</text><text if="kb.h20" fill="ink" radius="3" font="label-b" align="center">{kb.c20}</text></stack><stack w="*"><text if="!kb.h21" font="label-b" align="center">{kb.c21}</text><text if="kb.h21" fill="ink" radius="3" font="label-b" align="center">{kb.c21}</text></stack><stack w="*"><text if="!kb.h22" font="label-b" align="center">{kb.c22}</text><text if="kb.h22" fill="ink" radius="3" font="label-b" align="center">{kb.c22}</text></stack><stack w="*"><text if="!kb.h23" font="label-b" align="center">{kb.c23}</text><text if="kb.h23" fill="ink" radius="3" font="label-b" align="center">{kb.c23}</text></stack><stack w="*"><text if="!kb.h24" font="label-b" align="center">{kb.c24}</text><text if="kb.h24" fill="ink" radius="3" font="label-b" align="center">{kb.c24}</text></stack></row>
  </stack>
  <stack h="21" margin="0 4" if="kb.v3">
    <row pad="2 2"><stack w="*"><text if="!kb.h30" font="label-b" align="center">{kb.c30}</text><text if="kb.h30" fill="ink" radius="3" font="label-b" align="center">{kb.c30}</text></stack><stack w="*"><text if="!kb.h31" font="label-b" align="center">{kb.c31}</text><text if="kb.h31" fill="ink" radius="3" font="label-b" align="center">{kb.c31}</text></stack><stack w="*"><text if="!kb.h32" font="label-b" align="center">{kb.c32}</text><text if="kb.h32" fill="ink" radius="3" font="label-b" align="center">{kb.c32}</text></stack><stack w="*"><text if="!kb.h33" font="label-b" align="center">{kb.c33}</text><text if="kb.h33" fill="ink" radius="3" font="label-b" align="center">{kb.c33}</text></stack><stack w="*"><text if="!kb.h34" font="label-b" align="center">{kb.c34}</text><text if="kb.h34" fill="ink" radius="3" font="label-b" align="center">{kb.c34}</text></stack></row>
  </stack>
  <stack h="21" margin="0 4" if="kb.v4">
    <row pad="2 2"><stack w="*"><text if="!kb.h40" font="label-b" align="center">{kb.c40}</text><text if="kb.h40" fill="ink" radius="3" font="label-b" align="center">{kb.c40}</text></stack><stack w="*"><text if="!kb.h41" font="label-b" align="center">{kb.c41}</text><text if="kb.h41" fill="ink" radius="3" font="label-b" align="center">{kb.c41}</text></stack><stack w="*"><text if="!kb.h42" font="label-b" align="center">{kb.c42}</text><text if="kb.h42" fill="ink" radius="3" font="label-b" align="center">{kb.c42}</text></stack><stack w="*"><text if="!kb.h43" font="label-b" align="center">{kb.c43}</text><text if="kb.h43" fill="ink" radius="3" font="label-b" align="center">{kb.c43}</text></stack><stack w="*"><text if="!kb.h44" font="label-b" align="center">{kb.c44}</text><text if="kb.h44" fill="ink" radius="3" font="label-b" align="center">{kb.c44}</text></stack></row>
  </stack>
  <stack h="21" margin="0 4" if="kb.v5">
    <row pad="2 2"><stack w="*"><text if="!kb.h50" font="label-b" align="center">{kb.c50}</text><text if="kb.h50" fill="ink" radius="3" font="label-b" align="center">{kb.c50}</text></stack><stack w="*"><text if="!kb.h51" font="label-b" align="center">{kb.c51}</text><text if="kb.h51" fill="ink" radius="3" font="label-b" align="center">{kb.c51}</text></stack><stack w="*"><text if="!kb.h52" font="label-b" align="center">{kb.c52}</text><text if="kb.h52" fill="ink" radius="3" font="label-b" align="center">{kb.c52}</text></stack><stack w="*"><text if="!kb.h53" font="label-b" align="center">{kb.c53}</text><text if="kb.h53" fill="ink" radius="3" font="label-b" align="center">{kb.c53}</text></stack><stack w="*"><text if="!kb.h54" font="label-b" align="center">{kb.c54}</text><text if="kb.h54" fill="ink" radius="3" font="label-b" align="center">{kb.c54}</text></stack></row>
  </stack>
  <spacer h="5"/>
  <row h="18" margin="1 8" gap="6">
    <stack w="*" if="kb.a0"><text if="!kb.ah0" font="small" align="center" border="1" radius="4">{kb.a0}</text><text if="kb.ah0" fill="ink" radius="4" font="small-b" align="center">{kb.a0}</text></stack>
    <stack w="*" if="kb.a1"><text if="!kb.ah1" font="small" align="center" border="1" radius="4">{kb.a1}</text><text if="kb.ah1" fill="ink" radius="4" font="small-b" align="center">{kb.a1}</text></stack>
  </row>
  <row h="18" margin="1 8" gap="6">
    <stack w="*" if="kb.a2"><text if="!kb.ah2" font="small" align="center" border="1" radius="4">{kb.a2}</text><text if="kb.ah2" fill="ink" radius="4" font="small-b" align="center">{kb.a2}</text></stack>
    <stack w="*" if="kb.a3"><text if="!kb.ah3" font="small" align="center" border="1" radius="4">{kb.a3}</text><text if="kb.ah3" fill="ink" radius="4" font="small-b" align="center">{kb.a3}</text></stack>
  </row>
  <spacer h="*"/>
  <text h="13" font="small" align="center">{hint}</text>
  <spacer h="3"/>
</screen>
