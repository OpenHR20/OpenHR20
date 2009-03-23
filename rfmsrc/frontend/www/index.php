<?php
$ts=microtime(true);

include "common.php";

// just test
include "contend/timers.php";

echo "<div>duration ".(microtime(true)-$ts)."</div>\n";
