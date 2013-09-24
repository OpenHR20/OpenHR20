<?php

class contend_trace extends contend {
  public $protect=true;

  public function view() {
    global $db;
    $result = $db->query("SELECT * FROM trace WHERE addr=$this->addr AND idx=255");
    $row = $result->fetchArray();
    if ($row)
      include 'trace_layouts/'.sprintf("%02x",$row['value']).'.php';

    $result = $db->query("SELECT * FROM trace WHERE addr=$this->addr ORDER BY idx");
    echo ('<div><a href="?page=queue&read_trace=1&addr='.$this->addr.'">Make refresh requests for all values</a></div>');
    echo ('<table><tr><th>name</th><th>idx</th><th>value</th><th>last_update</th><th>description</th></tr>');  
    while ($row = $result->fetchArray()) {
	printf("<tr><td>%s</td><td>0x%02x</td><td>0x%04x</td><td>%s</td><td>%s</td></tr>\n"
	    ,$trace_layout_ids_double[$row['idx']][0]
	    ,$row['idx'],$row['value'],format_time($row['time'])
	    ,$trace_layout_ids_double[$row['idx']][1]);
    }
    echo "</table>";
  }
}

