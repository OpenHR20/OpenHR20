<pre>
<?php

$db = new SQLiteDatabase("/usb/home/db.sqlite");

$result = $db->query("SELECT * FROM log");

while ($row = $result->fetch()) {
    echo $row['id']." time ".$row['time']." line ".$row['data']."\n"; 
}

?>
</pre>