<?php
class contend_raw_command_queue extends contend {
  public function controller() {
   global $db,$_GET;
   if (isset($_GET['data'])) {
	$db->query("INSERT INTO command_queue (time,addr,data) VALUES ("
	.time().','.$this->addr.',"'.$_GET['data'].'");');    
    }

    $delete_id=(int) ($_GET["delete_id"]);
    if ($delete_id>0) {
	$db->query("DELETE FROM command_queue WHERE id=$delete_id");
    }
    return null;
  }
  
  public function view() {
    global $db;
    $result = $db->query("SELECT * FROM command_queue ORDER BY time");

    echo '
    <h1>Add new command</h1>
    <div><form method="get">
    <div>
	Command:<input type="text" name="data" size="20" value="">
	<input type="hidden" name="addr" value="'.$this->addr.'">
	<input type="hidden" name="page" value="raw_command_queue">
    </div>
    <input type="submit" value="add">
    </form></div>
    <h1>Dump queue</h1>
    ';

    echo "<table>\n";
    echo "<tr><th>addr</th><th>time</th><th>data</th><th>send</th><th></th></tr>";
    while ($row = $result->fetchArray()) {
	echo "<tr><td>".$row['addr']."</td>";
	echo "<td>".format_time($row['time'])."</td>";
	echo "<td>".$row['data']."</td>";
	echo "<td>".$row['send']."</td>";
	echo '<td><a href="?page=raw_command_queue&delete_id='.$row['id'].'">delete</a></td></tr>';
    }
    echo "</table>\n";
  }
}