<?php

function format_time($timestamp) {
    return date("Y-m-d H:i:s",$timestamp);
}

$db = new SQLiteDatabase("/usb/home/db.sqlite");


if (isset($_GET['addr']) && isset($_GET['addr'])) {
    $db->query("INSERT INTO command_queue (time,addr,data,weight) VALUES ("
    .time().','.(int)($_GET['addr']).',"'.$_GET['data'].'",3);');    
}

$result = $db->query("SELECT * FROM command_queue ORDER BY time");

echo '
<h1>Add new command</h1>
<div><form method="get">
<div>
    Addr:<input type="text" name="addr" size="2" value="">
    Command:<input type="text" name="data" size="20" value="">
</div>
<input type="submit" value="add">
</form></div>
<h1>Dump queue</h1>
';

echo "<table border=2>\n";
echo "<tr><th>addr</th><th>time</th><th>data</th><th>weight</th><th>send</th></tr>";
while ($row = $result->fetch()) {
    echo "<tr><td>".$row['addr']."</td>";
    echo "<td>".format_time($row['time'])."</td>";
    echo "<td>".$row['data']."</td>";
    echo "<td>".$row['weight']."</td>";
    echo "<td>".$row['send']."</td></tr>";
}
echo "</table>\n";
