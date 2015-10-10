<?php

class contend_history extends contend {
  
  public function view() {
    global $db,$_GET;

    $limit = (int)($_GET['limit']);
    if ($limit<=0) $limit=50;
    $offset = (int)($_GET['offset']);
    if ($offset<0) $offset=0;
	$order=' ORDER BY time DESC';

    $result = $db->query("SELECT * FROM log WHERE addr=$this->addr ORDER BY time DESC LIMIT $offset,$limit");

    if ($offset>0) echo "<a href=\"?page=history&addr=$this->addr&offset=".($offset-$limit)."&limit=$limit\">previous $limit</a>";
    echo " <a href=\"?page=history&addr=$this->addr&offset=".($offset+$limit)."&limit=$limit\">next $limit</a>";

    echo "<table>\n";
    echo "<tr><th>time</th><th>mode</th><th>valve</th><th>real</th><th>wanted</th><th>battery</th>";
    echo "<th>error</th><th>window</th><th>force</th></tr>";
    while ($row = $result->fetchArray()) {
	echo "<tr><td>".format_time($row['time'])."</td>";
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
  }
}

