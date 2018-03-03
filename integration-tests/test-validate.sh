#!/bin/sh
cat record.out | grep -v -E 'TXGQ|RECORD' > record.normalized
cat replay.out | grep -v -E 'REPLAY' > replay.normalized
diff record.normalized replay.normalized
retval=$?
echo $retval
return $retval
