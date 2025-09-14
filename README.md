# 1588 one-step hardware timestamping

```sh
gm=hpscrpitsn0
ei=hpscrpitsn1
intf=eth1

curl -LO https://github.com/richardcochran/linuxptp/raw/refs/tags/v4.2/configs/gPTP.cfg
scp -O gPTP.cfg $gm:
scp -O gPTP.cfg $ei:

ssh $gm tcpdump -i $intf -w $gm.pcap &
ssh $ei tcpdump -i $intf -w $ei.pcap &

ssh $gm ptp4l -f gPTP.cfg -i $intf --twoStepFlag 0 --BMCA noop --serverOnly 1 --priority1 0 --priority2 0 &
ssh $ei ptp4l -f gPTP.cfg -i $intf --twoStepFlag 0 --BMCA noop --clientOnly 1 --step_threshold 1 &

sleep 10

jobs -p | xargs kill
```