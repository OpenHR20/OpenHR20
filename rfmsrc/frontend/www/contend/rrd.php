<?php

class contend_rrd extends contend {
  public $protect=true;

  public function view() {
        global $PLOTS_DIR, $RRD_DAYS, $room_name;
	$ids = array ();
	if ($this->addr>0)
		$ids[] = $this->addr;
	else {
        	foreach ($room_name as $k=>$v) {
			$ids[] = $k;
		}	
	}
	foreach ($RRD_DAYS as $days) {
        	echo '<div>RRD for last '.$days.' days';
        	foreach ($ids as $id) {
        	        $rrdgraph = $PLOTS_DIR.'/openhr20_'.$id.'_'.$days.'.png';
        	        if (file_exists ($rrdgraph)) {
        	          echo '<p>';
        	          echo '<div>'.$room_name[$id].'</div>';
        	          echo '<img src="'.$rrdgraph.'"/>';
        	          echo '</p>';
        	        }
        	} 
		echo '</div>';
	}
  }
}

