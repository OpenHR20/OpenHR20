<?php

class contend_queue extends contend {
  public $warning=false;
  
  public function add_to_queue ($r) {
      global $db,$_GET;
      if($r==null) return;
      $db->query("BEGIN TRANSACTION");
      $t = time();
      foreach ($r as $k=>$i) {
	if (is_array($i)) {
	  foreach ($i as $j) {
	    $db->query("INSERT INTO command_queue (time,addr,data) VALUES ("
		.$t.','.$k.",'$j');");
	  }
	} else {
	  $db->query("INSERT INTO command_queue (time,addr,data) VALUES ("
	    .$t.','.$this->addr.",'$i');");
	}
      }
      $db->query("COMMIT");
  }


  public function controller() {
      global $db,$_GET;
      $cmd = array();
      // timmers

      if ($_GET['read_timers']==1) {
	for ($i=0; $i<8; $i++) {
	    for ($j=0; $j<8; $j++) {
		$cmd[]="R$i$j";
	    }
	}
      }

      if ($_GET['read_eeprom']==1) {
	$cmd[] = "Gff";
	for ($i=0; $i<0x36; $i++) {
	    $cmd[] = sprintf ("G%02x",$i);
	}
      }

      if ($_GET['read_trace']==1) {
	$cmd[] = "Tff";
	for ($i=0; $i<=0x0d; $i++) {
	    $cmd[] = sprintf ("T%02x",$i);
	}	
      }

      if ($_GET['read_info']==1) {
	$cmd[] = "D";
	$cmd[] = "V";
      }

      return $cmd;
  }

  public function view() {
    global $db,$refresh_value;
    if ($this->addr>0) 
      $result = $db->query("SELECT count(*) AS cnt FROM command_queue WHERE addr=$this->addr");
    else
      $result = $db->query("SELECT count(*) AS cnt FROM command_queue");
    $row = $result->fetchArray();
    if ($row['cnt']>0) {
      echo '<h1>waiting commands: ';
      echo $row['cnt']."</h1>";
    } else {
      echo '<h1>No waiting commands</h1>';
    }
    if ($this->warning) {
      echo "<div><strong class=\"warning\">This page can't be displayed during command pending.</strong></div>";
      echo "<div>Automatic refresh every $refresh_value second.</div>";
    }
  }
}
