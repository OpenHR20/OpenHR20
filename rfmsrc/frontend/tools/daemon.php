<?php
function weights($char) {
    $weights_table = array (
        'D' => 10,
        'S' => 4,
        'W' => 4,
        'G' => 2,
        'R' => 2
    );
    if (isset($weights_table[$char]))
        return $weights_table[$char];
    else 
        return 10;
}

$db = new SQLiteDatabase("/usb/home/db.sqlite");
$db->query("PRAGMA synchronous=OFF");

$fp=fsockopen("192.168.62.230",3531);
//$fp=fopen("php://stdin","r"); 
//$fp=fopen("/dev/ttyS1","w+"); 

//while(($line=stream_get_line($fp,256,"\n"))!=FALSE) {
while(($line=fgets($fp,256))!==FALSE) {
    $line=trim($line);
    if ($line == "") continue; // ignore empty lines
    $debug=true;

    $ts=microtime(true);
    if ($line{0}=='(' && $line{3}==')') {
	   $addr = hexdec(substr($line,1,2));
    } else if ($line{0}=='<' && $line{3}=='>') {
	   $addr = hexdec(substr($line,1,2));
	   $db->query("DELETE FROM command_queue WHERE id=(SELECT id FROM command_queue WHERE addr=$addr AND send>0 ORDER BY send LIMIT 1)");
    } else {
	   $addr=0;
    }
    
    if ($line=="RTC?") {
    	list($usec, $sec) = explode(" ", microtime());
    	$items = getdate($sec);
    	$time = sprintf("H%02x%02x%02x%02x\n",
    	    $items['hours'], $items['minutes'], $items['seconds'], round($usec*100));
    	$date = sprintf("Y%02x%02x%02x\n",
    	    $items['year']-2000, $items['mon'], $items['mday']);
    	echo $time ." ". $date;
    	fwrite($fp,$time); fwrite($fp,$date);
    	$debug=false;
    } else if ($line=="OK") {
        $debug=false;
    } else if (($line=="N0?") || ($line=="N1?")) {
        $result = $db->query("SELECT addr,count(*) AS c FROM command_queue GROUP BY addr ORDER BY c DESC");
        // $result = $db->query("SELECT addr,count(*) AS c FROM command_queue WHERE send=0 GROUP BY addr ORDER BY c DESC");
    	$req = array(0,0,0,0);
    	$v = "O00\n";
        while ($row = $result->fetch()) {
            $addr = $row['addr'];
            if (($addr>0) && ($addr<30)) {
                unset($v);
                /*if (($line=="N1?")&&($row['c']>20)) {
                    $v=sprintf("O%02x\n",$addr);
                    break;
                }*/
                $req[(int)$addr/8] += (int)pow(2,($addr%8));
            }
        }
        if (!isset($v)) $v = sprintf("P%02x%02x%02x%02x\n",$req[0],$req[1],$req[2],$req[3]);
        echo $v; fwrite($fp,$v);
        //fwrite($fp,"P14000000\n");
        $debug=false;
    } else {
    	if ($addr>0) {
    	  if ($line{4}=='?') {
    	    $debug=false;
    	    // echo "data req addr $addr\n";
    	    $db->query("BEGIN TRANSACTION");
    	    $result = $db->query("SELECT id,data FROM command_queue WHERE addr=$addr ORDER BY time LIMIT 20");
    	    $weight=0;
    	    $bank=0;
    	    $send=0;
    	    while ($row = $result->fetch()) {
    	       $cw = weights($row['data']{0});
    	       $weight += $cw;
               weights($row['data']{0});
    	       if ($weight>10) {
                    if (++$bank>=3) break;
                    $weight=$cw;
               }
    	       $r = sprintf("(%02x-%x)%s\n",$addr,$bank,$row['data']);
    	       fwrite($fp,$r);
               echo $r;
               $send++;
               $db->query("UPDATE command_queue SET send=$send WHERE id=".$row['id']);
            }
    	    $db->query("COMMIT");
    
    	    //$debug=false;
    	  } else if ($line{5}=='[' && $line{8}==']' && $line{9}=='=') {
    	    $idx=hexdec(substr($line,6,2));
    	    $value=hexdec(substr($line,10));
    	    switch ($line{4}) {
    	    case 'G':
    	    case 'S':
    		$table='eeprom';
    		break;
    	    case 'R':
    	    case 'W':
    		$table='timers';
    		break;
    	    case 'T':
    		$table='trace';
    		break;
    	    default:
    		$table=null;
    	    }
    	    echo " table $table\n";
    	    if ($table!==null) {
    	      $db->query("UPDATE $table SET time=".time().",value=$value WHERE addr=$addr AND idx=$idx");
    	      $changes=$db->changes();
    	      if ($changes==0)
    	        $db->query("INSERT INTO $table (time,addr,idx,value) VALUES (".time().",$addr,$idx,$value)");
    	    }
    	  } else if ($line{4}=='D' && $line{5}==' ') {
    	    $items = split(' ',$line);
    	    unset($items[0]);
    	    $t=0;
    	    $data=array();
    	    foreach ($items as $item) {
                switch ($item{0}) {
                    case 'm':
                        $t+=60*(int)(substr($item,1));
                        break;
                    case 's':
                        $t+=(int)(substr($item,1));
                        break;
                    case 'A':
                        $data['mode']='AUTO';
                        break;
                    case 'M':
                        $data['mode']='MANU';
                        break;
                    case 'V':
                        $data['valve']=(int)(substr($item,1));
                        break;
                    case 'I':
                        $data['real']=(int)(substr($item,1));
                        break;
                    case 'S':
                        $data['wanted']=(int)(substr($item,1));
                        break;
                    case 'B':
                        $data['battery']=(int)(substr($item,1));
                        break;
                    case 'E':
                        $data['error']=hexdec(substr($item,1));
                        break;
                    case 'W':
                        $data['window']=1;
                        break;
                    case 'X':
                        $data['force']=1;
                        break;
                }
    	    }
            $vars=""; $val="";
            foreach ($data as $k=>$v) {
                $vars.=",".$k;
                if (is_int($v)) $val.=",".$v;
                else $val.=",'".$v."'";
            }
            $time = time();
            if (($time % 3600)<$t) $time-=3600;
            $time = (int)($time/3600)*3600+$t;
        	$db->query("INSERT INTO log (time,addr$vars) VALUES ($time,$addr$val)\n");
    	  }
    	}
    }
    
    
    if ($debug) { //debug log
    	echo $line."\n"; 
	$db->query("INSERT INTO debug_log (time,addr,data) VALUES (".time().",$addr,\"$line\")");
	//echo "         duration ".(microtime(true)-$ts)."\n";
    }
} 
