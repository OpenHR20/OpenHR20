<?php

class contend_timers extends contend {

  public $protect = true;

  private $timers=array();
  private $timer_mode;
  private $temperatures=array();
  private $layout_names;

  function __construct($addr) {
      global $db;
      parent::__construct($addr);
      $result = $db->query("SELECT * FROM eeprom WHERE addr=$this->addr AND idx=255");
      $row = $result->fetchArray();
      if ($row) {
	include 'ee_layouts/'.sprintf("%02x",$row['value']).'.php';
	$this->layout_names = $layout_names;
      }
      $this->read_db();
      $this->protect=true;
  }
  
  private function read_db() {
    global $db,$timer_names,$symbols;
    $result = $db->query("SELECT idx,value FROM timers WHERE addr=$this->addr");

    while ($row = $result->fetchArray()) {
	$d = ($row['idx'])>>4;
	$t = ($row['idx'])&0xf;
	$this->timers[$d][$t]=$row['value'];
    }
    $this->timer_mode=get_config($this->layout_names,$this->addr,'timer_mode');
    
    for ($i=0;$i<4;$i++) {
      $c = get_config($this->layout_names,$this->addr,'temperature'.$i);
      if ($c != 'NA') $this->temperatures[$i] = $c;
    }
    
  }

  public function controller() {
  
    if (!isset($_POST['temp_0'])) return;
    $cmd=null;
    //timmer table
    for ($i=0; $i<8; $i++) {
      for ($j=0; $j<8; $j++) {
	$x = $this->timers[$i][$j];
	$h = (int)(($x&0xfff)/60);
	$m = (int)(($x&0xfff)%60);
	$t = $x>>12;
	if ($_POST["timer_v_$i$j"] == '') {
	  $time[0] = (int)((0xfff)/60);
	  $time[1] = (int)((0xfff)%60);
	} else {
	  $time = explode(":",trim($_POST["timer_v_$i$j"]));
	}
	$type = (int)($_POST["timer_t_$i$j"]);
	if(( $type != $t)  || ((int)($time[0]) != $h) || ((int)($time[1]) != $m)) {
	  $cmd[] = "W$i$j".sprintf("%04x",($type<<12)|((int)($time[0])*60+(int)($time[1])));
	  // $cmd[] = "R$i$j";
	}
      }
    }
    // temperatures
    for ($i=0;$i<4;$i++) {
      $v = (int)($_POST["temp_$i"]*2);
      if ($this->temperatures[$i] != $v) {
	$cmd[] = sprintf("S%02x%02x",$this->layout_names["temperature$i"],$v);
	// $cmd[] = sprintf("G%02x",$this->layout_names["temperature$i"]);
      }
    }
    // mode
    $timer_mode = (int)$_POST['timer_mode'];
    if ($timer_mode < 0) $timer_mode = 0;
    if ($timer_mode > 1) $timer_mode = 1;
    if ($timer_mode != $this->timer_mode) {
      $cmd[] = sprintf("S%02x%02x",$this->layout_names['timer_mode'],$timer_mode);
      // $cmd[] = sprintf("G%02x",$this->layout_names['timer_mode']);
    }
    $this->timer_mode=get_config($this->layout_names,$this->addr,'timer_mode');
    
    // foreach($cmd as $c) echo "<div>$c</div>";
    return $cmd;
  }

  private function table_row($i) {
    global $timer_names,$symbols;
    echo '<tr><td>'.$timer_names[$i].'</td>';
    for ($j=0; $j<8; $j++) {
	echo '<td>';
	if (isset($this->timers[$i][$j])) {
	  $x = $this->timers[$i][$j];
	  $h = (int)(($x&0xfff)/60);
	  $v = $x>>12;
	  echo '<select name="timer_t_'.$i.$j.'">';
	  for($ii=0;$ii<4;$ii++) {
	    echo '<option value="'.$ii.'"'.(($v==$ii)?' selected':'').'>'.$symbols[$ii].'</option><br />';
	  }
	  echo '</select>';
	  echo '<input type="text" name="timer_v_'.$i.$j.'" maxlength="5" size="6" value="';
	      if ($h < 24)
		  printf("%02d:%02d",$h,(int)(($x&0xfff)%60));
	  echo '">';
	} else {
	  echo ' <strong>NA</strong>';
	}
	echo '</td>';
    }
    echo '</tr>';
  }
  
  public function view() {
      global $db,$timer_names,$symbols;

      echo ('<div><a href="?page=queue&read_timers=1&read_eeprom=1&addr='.$this->addr.'">Make refresh requests for all values</a></div>');
      echo '<form method="post" action="?page=timers&amp;addr='.$this->addr.'" />';
      
      echo "<h2>Preset temperatures</h2><table><tr>";
      for ($ii=0;$ii<4;$ii++) {
	echo "<td>".$symbols[$ii];
	if (isset($this->temperatures[$ii])) {
	  echo ' <input type="text" name="temp_'.$ii.'" maxlength="5" size="6" value="';
	  echo sprintf("%.1f",$this->temperatures[$ii]/2);
	  echo '" />&deg;C';
	} else {
	  echo ' <strong>NA</strong>';
	}
	echo '</td>';
      }
      echo "</tr></table>";

      $timers=array();
      
      echo '<h2>Timer table</h2>';
      echo '<div><input type="radio" name="timer_mode" value="0"'.(($this->timer_mode==0)?' checked="checked"':'').'>';
      echo 'One program for every day</div>';
      echo '<table>';
	$this->table_row(0);
      echo '</table>';

      echo '<div><input type="radio" name="timer_mode" value="1"'.(($this->timer_mode==1)?' checked="checked"':'').'>';
      echo 'Individual program for week day</div>';
      echo '<table>';
      for ($i=1; $i<8; $i++) {
	$this->table_row($i);
      }
      echo '</table>';
      echo '<input type="reset" value="Reset">';
      echo '<input type="submit" value="Submit">';
      echo "</form>";
  }
}
        
