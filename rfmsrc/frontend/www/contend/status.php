<?php
include_once "error.php";

class contend_status extends contend {
  public $protect = true;

  public function controller() {
    global $db,$room_name;
    $cmd=null;
    if ($_POST['type'] == 'addr') {
      $result = $db->query("SELECT * FROM log WHERE addr=$this->addr ORDER BY TIME DESC LIMIT 1");
      // foreach ($_POST as $k=>$p) echo "<div>$k => $p</div>";
      if ($row = $result->fetch()) {
	if ((isset($_POST['auto_mode']) && ($row['mode']!=$_POST['auto_mode']))) {
	  switch ($_POST['auto_mode']) {
	    case 'AUTO':
	      $cmd[] = "M01";
	      break;
	    case 'MANU':
	      $cmd[] = "M00";
	      break;
	  }
	}
	$temp = (int)($_POST['w_temp']*2);
	if (($_POST['w_temp'] != '') && ($temp>=9) && ($temp<=61) && ((int)($row['wanted']/50)!=$temp)) {
	  $cmd[]=sprintf("A%02x",$temp);
	} else {
	  if ($cmd) $cmd[]='D'; // update status
	}
      }  
    } else if ($_POST['type'] == 'all') {
      foreach ($room_name as $k=>$v) {
	$result = $db->query("SELECT * FROM log WHERE addr=$k ORDER BY time DESC LIMIT 1");
	if ($row = $result->fetch()) {
	  if ((isset($_POST["auto_mode_$k"]) && ($row['mode']!=$_POST["auto_mode_$k"]))) {
	    switch ($_POST["auto_mode_$k"]) {
	      case 'AUTO':
		$cmd[$k][] = "M01";
		break;
	      case 'MANU':
		$cmd[$k][] = "M00";
		break;
	    }
	  }
	  $temp = (int)($_POST["w_temp_$k"]*2);
	  if (($_POST["w_temp_$k"] != '') && ($temp>=9) && ($temp<=61) && ((int)($row['wanted']/50)!=$temp)) {
	    $cmd[$k][]=sprintf("A%02x",$temp);
	  } else {
	    if ($cmd[$k]) $cmd[$k][]='D'; // update status
	  }
	}  
      }
    }
    return $cmd;
  }  
  
  public function view() {
    global $db,$room_name,$chart_hours;
    if ($this->addr > 0) {
      echo ('<div><a href="?page=queue&read_info=1&addr='.$this->addr.'">Make refresh requests for all values</a></div>');
      $result = $db->query("SELECT * FROM log WHERE addr=$this->addr ORDER BY time DESC LIMIT 1");

      if ($row = $result->fetch()) {
	echo '<form method="post" action="?page=status&amp;addr='.$this->addr.'" />';
	echo "<div>Last update: ".format_time($row['time'])."</div>";
	echo "<div>Mode: ";
	echo   '<input type="radio" name="auto_mode" value="AUTO"'.(($row['mode']=='AUTO')?' checked="checked"':'').'> AUTO';
	echo   '<input type="radio" name="auto_mode" value="MANU"'.(($row['mode']=='MANU')?' checked="checked"':'').'> MANU';
	echo '</div>';
	echo "<div>Valve position: ".$row['valve']."</div>";
	echo "<div>Real temperature: ".($row['real']/100)."&deg;C</div>";
	echo "<div>Wanted temperature: ";
	echo   '<input type="text" name="w_temp" maxlength="5" size="6" value="'.($row['wanted']/100).'" />&deg;C';
	echo '</div>';
	echo "<div>Battery: ".($row['battery']/1000)."V</div>";
	if ($row['error']) echo "<div>error:".$row['error']."</div>";
	echo "<div>Window: ".(($row['window'])?"open":"close")."</div>";
	echo '<input type="hidden" name="type" value="addr">';
	echo '<input type="reset" value="Reset">';
	echo '<input type="submit" value="Submit">';
	echo "</form>";
      }
      echo "<div><img src=\"/chart.php?addr=$this->addr&real=1&wanted=1&valve=1&hours=$chart_hours\" \></div>";
      echo '<div><span style="color: red;">Real temperature</span> <span style="color: green;">Wanted temperature</span> <span style="color: blue;">Valve position</span></div>';
      $result = $db->query("SELECT * FROM versions WHERE addr=$this->addr LIMIT 1");
      if ($row = $result->fetch()) {
	echo "<div>SW version: ".$row['data']."</div>";
	echo "<div>SW version last update: ".format_time($row['time'])."</div>";
      }

    } else {
//	this SELECT is teoreticaly best practice, but unaceptable slow
//      $result = $db->query("SELECT * FROM log WHERE id IN (SELECT min(id) from log GROUP BY addr) ORDER BY addr");
//      $result = $db->query("SELECT * FROM log l,(SELECT min(id) AS m from log GROUP BY addr) AS m WHERE m.m=l.id");
      
	echo '<form method="post" action="?page=status&amp;addr='.$this->addr.'" /><table>';
	echo '<tr><th>valve</th><th>Last update</th><th>Mode</th><th>Valve [%]</th><th>Real [&deg;C]</th>'
	    .'<th>Wanted [&deg;C]</th><th>Battery</th><th>Error</th><th>Window</th></tr>';
	foreach ($room_name as $k=>$v) {
	  $result = $db->query("SELECT * FROM log WHERE addr=$k ORDER BY time DESC LIMIT 1");
	  echo "<tr><td><a href=\"?page=status&amp;addr=$k\">$v</a></td>";
	  if ($row = $result->fetch()) {
	    $age=time()-$row['time'];
	    if ($age > $GLOBALS['error_age']) {
        $age_t=' class="error"';
      } else if ($age > $GLOBALS['warning_age']) {
        $age_t=' class="warning"';
      }  else {
        $age_t='';
        // $age_t=' class="ok"';
      }
	    echo "<td$age_t>".format_time($row['time'])."</td><td>";
	    echo   '<input type="radio" name="auto_mode_'.$k.'" value="AUTO"'.(($row['mode']=='AUTO')?' checked="checked"':'').'> AUTO<br />';
	    echo   '<input type="radio" name="auto_mode_'.$k.'" value="MANU"'.(($row['mode']=='MANU')?' checked="checked"':'').'> MANU';
	    echo "</td><td>".$row['valve']."</td>";
	    echo "<td>".($row['real']/100)."&deg;C</td>";
	    echo '<td><input type="text" name="w_temp_'.$k.'" maxlength="5" size="6" value="'.($row['wanted']/100).'" /></td>';
      if ($GLOBALS['error_mask']['BAT_E'] & $row['error']) {
        $bat_c = ' class="error"'; 
      } else if ($GLOBALS['error_mask']['BAT_W'] & $row['error']) {
        $bat_c = ' class="warning"'; 
      } else {
        $bat_c = ''; 
      }
	    echo "<td$bat_c>".($row['battery']/1000)."V</td>";
	    echo "<td".((($row['error'])&$GLOBALS['error_show'])?' class="error"':'').">";
	      $errs=0;
        foreach ($GLOBALS['error_mask'] as $k=>$v) {
          if ($v & $row['error']) {
            if ($errs) echo "<br />";
            echo $k; 
            $errs++;
          } 
        }
      echo "</td>";
	    
	    echo "<td>".(($row['window'])?"open":"close")."</td>";
	  } else {
	    echo '<td class="error">NA</td><td>-</td><td>-</td><td>-</td><td>-</td><td>-</td><td>-</td><td>-</td>';
	  }
	  echo "</tr>";
	}
	echo '<input type="hidden" name="type" value="all">';
	echo '</table><input type="reset" value="Reset">';
	echo '<input type="submit" value="Submit">';
	echo "</form>";
     }  
  }
}
