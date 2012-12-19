<?php

class contend_eeprom extends contend {
  public $protect = true;

  public function controller() {
    global $db;
    $cmd=null;
    if (!isset($_POST['data_255'])) return;
    $result = $db->query("SELECT * FROM eeprom WHERE addr=$this->addr ORDER BY addr,idx");
    while ($row = $result->fetchArray()) {
      $fv = hexdec($_POST['data_'.$row['idx']]);
      if ($row['value']!=$fv) $cmd[]=sprintf("S%02x%02x",$row['idx'],$fv);
    }
    return $cmd;
  }
  
  public function view() {
    global $db;
    $result = $db->query("SELECT * FROM eeprom WHERE addr=$this->addr AND idx=255");
    $row = $result->fetchArray();
    if ($row)
      include 'ee_layouts/'.sprintf("%02x",$row['value']).'.php';

    $result = $db->query("SELECT * FROM eeprom WHERE addr=$this->addr ORDER BY addr,idx");

    echo ('<div><a href="?page=queue&read_eeprom=1&addr='.$this->addr.'">Make refresh requests for all values</a></div>');
    
    echo '<form method="post" action="?page=eeprom&amp;addr='.$this->addr.'" /><table>';

    echo ('<table><tr><th>idx</th><th>idx</th><th>value</th><th>last_update</th><th>description</th></tr>');  
    while ($row = $result->fetchArray()) {
	$label = $layout_ids[$row['idx']];
	$comment = $layout_ids_double[$row['idx']][1];
	printf('<tr><td>%s</td><td>0x%02x</td><td>0x<input type="text" name="data_%d" size="2" value="%02x"></td><td>%s</td><td>%s</td></tr>'
	    ,$label,$row['idx'],$row['idx'],$row['value'],format_time($row['time']),$comment);
    }
    echo '</table><input type="reset" value="Reset">';
    echo '<input type="submit" value="Submit">';
    echo "</form>";
  }
}