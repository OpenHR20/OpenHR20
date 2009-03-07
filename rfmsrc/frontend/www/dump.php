<?php

function format_time($timestamp) {
    return date("Y-m-d H:i:s",$timestamp);
}

$db = new SQLiteDatabase("/usb/home/db.sqlite");

$type = $_GET['type'];
echo "<h1>$type dump</h1>\n";

switch ($type) {

case 'eeprom':
case 'timers':
case 'trace':
    $order=' ORDER BY addr,idx';
    break;
case 'debug_log':
    $order=' ORDER BY time DESC';
    break;
case 'log':    
    $order=' ORDER BY time DESC';
    break;
default:
}

$result = $db->query("SELECT * FROM $type$order");

switch ($type) {

case 'eeprom':
case 'timers':
case 'trace':
    echo ('<table style="border:1px solid black;" border=2><tr><th>addr</th><th>idx</th><th>value</th><th>last_update</th></tr>');  
    while ($row = $result->fetch()) {
	printf("<tr><td>%d</td><td>%x</td><td>%x</td><td>%s</td></tr>\n"
	    ,$row['addr'],$row['idx'],$row['value'],format_time($row['time']));
    }
    echo "</table>";
    break;
case 'debug_log':
    echo "<pre>\n";
    while ($row = $result->fetch()) {
	if ($row['addr']>0) $x=sprintf("<%02d>",$row['addr']);
	else $x="    ";
        printf("%s %s %s\n",format_time($row['time']),$x,$row['data']);
    }
    echo "</pre>\n";
    break;
case 'log':    
    echo "<table border=2>\n";
    echo "<tr><th>addr</th><th>time</th><th>mode</th><th>valve</th><th>real</th><th>wanted</th><th>battery</th>";
    echo "<th>error</th><th>window</th><th>force</th></tr>";
    while ($row = $result->fetch()) {
        echo "<tr><td>".$row['addr']."</td>";
        echo "<td>".format_time($row['time'])."</td>";
        echo "<td>".$row['mode']."</td>";
        echo "<td>".$row['valve']."</td>";
        echo "<td>".($row['real']/100)."</td>";
        echo "<td>".($row['wanted']/100)."</td>";
        echo "<td>".($row['battery']/1000)."</td>";
        echo "<td>".$row['error']."</td>";
        echo "<td>".$row['window']."</td>";
        echo "<td>".$row['force']."</td></tr>";
    }
    echo "</table>\n";
    break;
default:
    echo "<pre>\n";
    while ($row = $result->fetch()) {
        print_r($row);
    }
    echo "</pre>\n";
}
