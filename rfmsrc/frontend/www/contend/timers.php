<?php

$symbols = array ('x','&#x263e;','&#x2600;','&#x263c;');

echo "<h1>$type dump</h1>\n";

if (isset($_GET['addr'])) {
    $where= ' WHERE addr='. (int)($_GET['addr']);
} else {
    $where='';
}

if (isset($_GET['read_addr'])) {
    $db->query("BEGIN TRANSACTION");
    for ($i=0; $i<8; $i++) {
        for ($j=0; $j<8; $j++) {
            $db->query("INSERT INTO command_queue (time,addr,data) VALUES ("
                .time().','.(int)($_GET['read_addr']).",'R$i$j');");
        }
    }
    $db->query("COMMIT");
}

$timers=array();

$result = $db->query("SELECT idx,value FROM timers $where");

    while ($row = $result->fetch()) {
        $d = ($row['idx'])>>4;
        $t = ($row['idx'])&0xf;
        $timers[$d][$t]=$row['value'];
    }

echo '<table style="border:1px solid black;" border=2>';
for ($i=0; $i<8; $i++) {
    echo '<tr>';
    for ($j=0; $j<8; $j++) {
        $x = $timers[$i][$j];
        $h = (int)(($x&0xfff)/60);
        $v = $x>>12;
        echo '<td>';
        echo $symbols[$v].'<input type="text" name="timer'.$i.$j.'" maxlength="5" size="6" value="';
            if ($h < 24)
                printf("%02d:%02d",$h,(int)(($x&0xfff)%60));
        echo '">';
        echo '</td>';
    }
    echo '</tr>';
}
echo '</table>';

        