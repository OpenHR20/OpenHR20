<?php

class contend_debug_log extends contend {
  
  public function view() {
    global $db,$_GET;

    $limit = (int)($_GET['limit']);
    if ($limit<=0) $limit=50;
    $offset = (int)($_GET['offset']);
    if ($offset<0) $offset=0;

    if ($this->addr) {
	$where= ' WHERE addr='. $this->addr;
    } else {
	$where='';
    }


    $result = $db->query("SELECT * FROM debug_log$where ORDER BY time DESC LIMIT $offset,$limit");
    
    echo "<div>";
    if ($offset>0) echo "<a href=\"?page=debug_log&addr=$this->addr&offset=".($offset-$limit)."&limit=$limit\">previous $limit</a>";
    echo " <a href=\"?page=debug_log&addr=$this->addr&offset=".($offset+$limit)."&limit=$limit\">next $limit</a>";

    echo "</div>";
    echo "<pre>\n";
    while ($row = $result->fetchArray()) {
	if ($row['addr']>0) $x=sprintf("<%02d>",$row['addr']);
	else $x="    ";
	printf("%s %s %s\n",format_time($row['time']),$x,$row['data']);
    }
    echo "</pre>\n";
  }
}

