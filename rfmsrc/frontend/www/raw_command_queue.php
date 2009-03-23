<?php

function format_time($timestamp) {
    return date("Y-m-d H:i:s",$timestamp);
}

$db = new SQLiteDatabase("/usb/home/db.sqlite");


if (isset($_GET['addr']) && isset($_GET['addr'])) {
    $db->query("INSERT INTO command_queue (time,addr,data) VALUES ("
    .time().','.(int)($_GET['addr']).',"'.$_GET['data'].'");');    
}

$delete_id=(int) ($_GET["delete_id"]);
if ($delete_id>0) {
    $db->query("DELETE FROM command_queue WHERE id=$delete_id");
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
echo "<tr><th>addr</th><th>time</th><th>data</th><th>send</th><th></th></tr>";
while ($row = $result->fetch()) {
    echo "<tr><td>".$row['addr']."</td>";
    echo "<td>".format_time($row['time'])."</td>";
    echo "<td>".$row['data']."</td>";
    echo "<td>".$row['send']."</td>";
    echo '<td><a href="?delete_id='.$row['id'].'">delete</a></td></tr>';
}
echo "</table>\n";
