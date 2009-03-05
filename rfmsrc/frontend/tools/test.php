<?php

$db = new SQLiteDatabase("/usb/home/db.sqlite");
$db->query("PRAGMA synchronous=OFF");
//$db->query("BEGIN TRANSACTION");
//	$db->query("COMMIT");
//	$db->query("BEGIN TRANSACTION");

$fp=fsockopen("192.168.62.230",3531);
//$fp=fopen("php://stdin","r"); 

//while(($line=stream_get_line($fp,256,"\n"))!=FALSE) {
while(($line=fgets($fp,65535))!==FALSE) { 
    $line=trim($line);
    $debug=true;

    $ts=microtime(true);
    if ($line{0}=='<' && $line{3}=='>') {
	$addr = hexdec(substr($line,1,2));
    } else {
	$addr=0;
    }
    
    if ($line=="RTC?") {
	list($usec, $sec) = explode(" ", microtime());
	$items = getdate($sec);
	$time = sprintf("H%02x%02x%02x%02x\n",
	    $items['hours'],
	    $items['minutes'],
	    $items['seconds'],
	    round($usec*100));
	$date = sprintf("Y%02x%02x%02x\n",
	    $items['year']-2000,
	    $items['mon'],
	    $items['mday']);
	echo $time;
	fwrite($fp,$time);
	echo $date;
	fwrite($fp,$date);
	$debug=false;
    } else {
	if ($addr>0) {
	  if ($line{4}=='?') {
	    $buf=hexdec(substr($line,5));
	    echo "data req addr $addr buffer $buf\n";
	    $debug=false;
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
	echo "         duration ".(microtime(true)-$ts)."\n";
    }
} 
